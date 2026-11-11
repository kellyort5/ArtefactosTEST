#include "esp_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"
#include "module_runtime.h"
#include "network.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_v2.h"
#include "sentinel.h"
#include "wire.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/time.h>
namespace sentinel {
uint64_t monotonic_ms() { return esp_timer_get_time() / 1000; }
static char device_id[65] = {}, boot_id[65] = {};
static Runtime runtime;
static Profile previous_profile = Profile::Idle;
static bool button_transition_pending = false;
static Network network;
static CommandGate gate;
static std::atomic<uint64_t> dropped_messages{0};
static ModuleSupervisor modules(runtime, network);
static ModuleOutputQueue module_outputs;
class CoreContext : public ModuleContext {
  Profile mode;
  ModuleLease lease;

public:
  explicit CoreContext(Profile p) : mode(p), lease(modules, p, network) {}
  uint64_t now_ms() const override { return monotonic_ms(); }
  ModuleResources &resources() override { return lease; }
  bool emit_event(const char *name, const char *severity, const char *details,
                  bool simulated) override {
    if (!modules.active(mode) || !name || !severity || !details ||
        strlen(name) >= 40 || strlen(severity) >= 12 ||
        strlen(details) > ModuleJsonBytes)
      return false;
    ModuleOutput out;
    out.mode = mode;
    out.simulated = simulated;
    strcpy(out.name, name);
    strcpy(out.severity, severity);
    strcpy(out.json, details);
    return module_outputs.push(out);
  }
  bool publish_metrics(uint32_t interval, const char *values) override {
    if (!modules.active(mode) || !values || strlen(values) > ModuleJsonBytes)
      return false;
    ModuleOutput out;
    out.mode = mode;
    out.metric = true;
    out.interval_ms = interval;
    strcpy(out.json, values);
    return module_outputs.push(out);
  }
};
static CoreContext escudo_context(Profile::Escudo),
    sanitizador_context(Profile::Sanitizador), cebo_context(Profile::Cebo);
static bool registration_open = false;
bool register_module(Mode mode, Module &m) {
  if (!registration_open)
    return false;
  auto &ctx = mode == Profile::Escudo        ? escudo_context
              : mode == Profile::Sanitizador ? sanitizador_context
                                             : cebo_context;
  bool ok = modules.add(mode, m, ctx);
  if (!ok)
    runtime.error = "resource_error";
  return ok;
}
static struct TransitionCommand {
  bool pending = false;
  Result result;
  Channel channel = Channel::Ble;
  uint32_t session = 0;
  uint8_t digest[32] = {};
} transition_command;
static bool has_oled = false, has_button = false;
static std::atomic<bool> has_time{false};
static std::atomic<uint64_t> message_seq{0};
static uint64_t passkey_until = 0;
static char shown_passkey[16] = {};
static SemaphoreHandle_t snapshot_lock;
static char snapshot[257] = {};
class NvsStore : public Store {
  nvs_handle_t handle = 0;
  esp_err_t last = ESP_OK;

public:
  bool ready = false;
  bool init() {
    last = nvs_flash_init_partition("event_log");
    if (last != ESP_OK)
      return false;
    last = nvs_open_from_partition("event_log", "ring", NVS_READWRITE, &handle);
    ready = last == ESP_OK;
    return ready;
  }
  bool read(size_t slot, char *out, size_t *n) override {
    if (!ready) {
      last = ESP_FAIL;
      return false;
    }
    char key[16];
    snprintf(key, sizeof(key), "slot%02u", static_cast<unsigned>(slot));
    last = nvs_get_blob(handle, key, out, n);
    return last == ESP_OK;
  }
  bool missing() const override { return last == ESP_ERR_NVS_NOT_FOUND; }
  bool write(size_t slot, const char *data, size_t n) override {
    if (!ready)
      return false;
    char key[16];
    snprintf(key, sizeof(key), "slot%02u", static_cast<unsigned>(slot));
    last = nvs_set_blob(handle, key, data, n);
    if (last == ESP_OK)
      last = nvs_commit(handle);
    return last == ESP_OK;
  }
};
static NvsStore store;
static Journal journal(store);
struct Request {
  bool command = false;
  uint8_t slot = 0;
  Channel channel = Channel::Ble;
  bool authorized = false, retained = false;
  uint32_t session = 0;
  Signal signal = {Kind::ShortPress};
};
static QueueHandle_t requests, busy_results;
struct BusyResult {
  Result result;
  Channel channel;
  uint32_t session;
  bool cache = false, lookup_only = false;
  uint64_t expiry = 0;
  char operation[32] = {};
  uint8_t digest[32] = {};
  const char *status = "rejected";
};
struct Slot {
  size_t size = 0;
  char bytes[MaxMessage + 1] = {};
};
static Slot slots[QueueSize];
static PendingCommands pending;
static portMUX_TYPE pool_lock = portMUX_INITIALIZER_UNLOCKED;
static Envelope context(char timestamp[33]) {
  Envelope e{device_id, boot_id, ++message_seq, monotonic_ms()};
  if (has_time) {
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    tm utc = {};
    gmtime_r(&tv.tv_sec, &utc);
    int year = utc.tm_year + 1900;
    if (year >= 1 && year <= 9999 &&
        strftime(timestamp + 4, 29, "-%m-%dT%H:%M:%SZ", &utc)) {
      timestamp[0] = '0' + year / 1000;
      timestamp[1] = '0' + (year / 100) % 10;
      timestamp[2] = '0' + (year / 10) % 10;
      timestamp[3] = '0' + year % 10;
      e.timestamp = timestamp;
      e.quality = "app_sync";
    }
  }
  e.version = 2;
  return e;
}
static bool send_raw(Channel ch, const char *json, size_t n, uint32_t session,
                     const char *suffix) {
  return ch == Channel::Ble
             ? ble_send(json, n, session)
             : network.publish(suffix, json, n, 1, !strcmp(suffix, "state"));
}
static bool send_payload(Channel ch, const char *type, cJSON *p,
                         uint32_t session = 0) {
  char timestamp[33], json[MaxMessage + 1];
  bool ok = serialize(envelope(context(timestamp), type, p), json);
  if (ok)
    ok = send_raw(ch, json, strlen(json), session,
                  !strcmp(type, "ack")       ? "acks"
                  : !strcmp(type, "event")   ? "events"
                  : !strcmp(type, "metrics") ? "metrics"
                                             : "state");
  return ok;
}
static void update_snapshot() {
  if (!snapshot_lock)
    return;
  xSemaphoreTake(snapshot_lock, portMAX_DELAY);
  snprintf(snapshot, sizeof(snapshot),
           "{\"version\":2,\"device_id\":\"%s\",\"boot_id\":\"%s\","
           "\"revision\":%llu,\"uptime_ms\":%llu,\"max_message_bytes\":2048}",
           device_id, boot_id,
           static_cast<unsigned long long>(runtime.revision),
           static_cast<unsigned long long>(monotonic_ms()));
  xSemaphoreGive(snapshot_lock);
}
bool metadata(char *out, size_t n) {
  if (!snapshot_lock)
    return false;
  xSemaphoreTake(snapshot_lock, portMAX_DELAY);
  size_t size = strlen(snapshot);
  bool ok = size && size < n;
  if (ok)
    memcpy(out, snapshot, size + 1);
  xSemaphoreGive(snapshot_lock);
  return ok;
}
bool post(Signal s) {
  if (!requests)
    return false;
  Request r;
  r.signal = s;
  bool ok = xQueueSend(requests, &r, 0) == pdTRUE;
  if (!ok)
    ++dropped_messages;
  return ok;
}
static void busy(Channel ch, const char *data, size_t n, bool authorized,
                 bool retained, uint32_t session) {
  if (!authorized || retained)
    return;
  Command c;
  if (c.parse_v2(data, n))
    return;
  char json[257];
  if (!metadata(json, sizeof(json)))
    return;
  auto meta = cJSON_Parse(json);
  uint64_t rev = number(meta, "revision");
  cJSON_Delete(meta);
  auto p = cJSON_GetObjectItemCaseSensitive(c.root, "payload");
  uint64_t now = monotonic_ms();
  if (strcmp(str(c.root, "device_id"), device_id) ||
      strcmp(str(c.root, "boot_id"), boot_id) ||
      strcmp(str(p, "target_boot_id"), boot_id) || c.expiry <= now ||
      c.expiry - now > 30000)
    return;
  Result r;
  strcpy(r.id, c.id);
  r.error = "busy";
  r.revision = rev;
  // BLE sends from its bounded TX queue. MQTT callbacks never touch the
  // resource manager.
  BusyResult report{r, ch, session};
  report.expiry = c.expiry;
  strcpy(report.operation, c.operation);
  report.cache = digest_command(c, report.digest);
  if (busy_results)
    xQueueSend(busy_results, &report, 0);
}
bool submit(Channel ch, const char *json, size_t n, bool auth, bool retained,
            uint32_t session) {
  if (!requests || !n || n > MaxMessage)
    return false;
  Command c;
  const char *validation = c.parse_v2(json, n);
  uint8_t hash[32] = {};
  bool identified = !validation && auth && !retained;
  auto payload = cJSON_GetObjectItemCaseSensitive(c.root, "payload");
  uint64_t now = monotonic_ms();
  if (identified)
    identified = !strcmp(str(c.root, "device_id"), device_id) &&
                 !strcmp(str(c.root, "boot_id"), boot_id) &&
                 !strcmp(str(payload, "target_boot_id"), boot_id) &&
                 c.expiry > now && c.expiry - now <= 30000 &&
                 digest_command(c, hash);
  int slot = -1;
  portENTER_CRITICAL(&pool_lock);
  slot = pending.admit(identified ? c.id : nullptr, hash, ch, session,
                       ble_session());
  portEXIT_CRITICAL(&pool_lock);
  if (slot == -2 || slot == -3) {
    BusyResult report;
    strcpy(report.result.id, c.id);
    report.channel = ch;
    report.session = session;
    report.expiry = c.expiry;
    strcpy(report.operation, c.operation);
    report.lookup_only = slot == -2;
    memcpy(report.digest, hash, 32);
    report.status = slot == -2 ? "accepted" : "rejected";
    report.result.error = slot == -3 ? "id_conflict" : nullptr;
    xQueueSend(busy_results, &report, 0);
    wipe(hash, sizeof(hash));
    return true;
  }
  wipe(hash, sizeof(hash));
  if (slot < 0) {
    ++dropped_messages;
    busy(ch, json, n, auth, retained, session);
    return false;
  }
  auto &s = slots[slot];
  memcpy(s.bytes, json, n);
  s.bytes[n] = 0;
  s.size = n;
  Request r;
  r.command = true;
  r.slot = slot;
  r.channel = ch;
  r.authorized = auth;
  r.retained = retained;
  r.session = session;
  if (xQueueSend(requests, &r, 0) == pdTRUE)
    return true;
  wipe(s.bytes, sizeof(s.bytes));
  portENTER_CRITICAL(&pool_lock);
  pending.release(slot);
  portEXIT_CRITICAL(&pool_lock);
  ++dropped_messages;
  busy(ch, json, n, auth, retained, session);
  return false;
}
static void state(Channel ch, uint32_t session = 0) {
  send_payload(ch, "state",
               module_state_payload(runtime, modules, ble_status(),
                                    network.wifi_state, network.mqtt_state,
                                    has_oled, has_button,
                                    store.ready && !journal.degraded),
               session);
}
static void publish_state() {
  if (!network.connected())
    return;
  char timestamp[33], json[MaxMessage + 1];
  if (serialize(envelope(context(timestamp), "state",
                         module_state_payload(
                             runtime, modules, ble_status(), network.wifi_state,
                             network.mqtt_state, has_oled, has_button,
                             store.ready && !journal.degraded)),
                json))
    network.publish("state", json, strlen(json), 1, true);
}
static void event(const char *name, const char *code = nullptr) {
  if (!store.ready) {
    runtime.error = "resource_error";
    return;
  }
  char timestamp[33], json[MaxMessage + 1];
  uint64_t cursor = journal.highest + 1;
  char details[96];
  snprintf(details, sizeof(details), "{\"previous_profile\":\"%s\"}",
           profile_name(previous_profile));
  if (!serialize(
          envelope(context(timestamp), "event",
                   module_event_payload(
                       cursor, name, runtime.profile, code,
                       !strcmp(name, "system.error")
                           ? "{\"resource\":\"system\"}"
                       : !strcmp(name, "system.profile_changed") ? details
                                                                 : "{}",
                       code ? "error" : "info")),
          json) ||
      !journal.append(json, strlen(json), cursor)) {
    runtime.error = "resource_error";
    return;
  }
  ble_send(json, strlen(json));
  network.publish("events", json, strlen(json), 1, false);
}
static bool page(Channel ch, const Result &r, uint32_t session) {
  if (!r.page)
    return true;
  for (size_t i = 0; i < r.count; ++i) {
    char json[MaxMessage + 1];
    size_t len = sizeof(json);
    if (!journal.read(r.cursors[i], json, &len) ||
        !send_raw(ch, json, len, session, "events"))
      return false;
  }
  return true;
}
static void handle(Request &r) {
  auto &slot = slots[r.slot];
  Command c;
  const char *error = c.parse_v2(slot.bytes, slot.size);
  Result result;
  result.revision = runtime.revision;
  if (!*c.id) { // Malformed orders without a usable correlation ID get only a
                // sanitized diagnostic.
    ESP_LOGW("sentinel", "invalid_message: uncorrelated input");
    return;
  }
  strcpy(result.id, c.id);
  if (error) {
    result.error = error;
    send_payload(r.channel, "ack", module_ack_payload(result, "rejected"),
                 r.session);
    return;
  }
  if (r.channel == Channel::Ble && r.session != ble_session())
    return;
  uint8_t hash[32] = {};
  if (!digest_command(c, hash)) {
    result.error = "resource_error";
    send_payload(r.channel, "ack", module_ack_payload(result, "rejected"),
                 r.session);
    return;
  }
  const Result *cached = nullptr;
  const char *channel_error =
      v2::admission(c, r.channel, runtime.profile, true, false);
  error = gate.check(c, device_id, boot_id, runtime.revision, monotonic_ms(),
                     r.authorized, r.retained, hash, &cached, channel_error);
  if (!error &&
      (!strcmp(c.operation, "configure_connectivity") ||
       !strcmp(c.operation, "set_time")) &&
      r.channel != Channel::Ble)
    error = "unauthorized";
  if (error) {
    result.error = error;
    if (!strcmp(error, "revision_conflict"))
      gate.remember(c, hash, result);
    send_payload(r.channel, "ack", module_ack_payload(result, "rejected"),
                 r.session);
    return;
  }
  if (transition_command.pending &&
      !strcmp(c.id, transition_command.result.id)) {
    result.error =
        memcmp(hash, transition_command.digest, 32) ? "id_conflict" : nullptr;
    send_payload(
        r.channel, "ack",
        module_ack_payload(result, result.error ? "rejected" : "accepted"),
        r.session);
    wipe(hash, sizeof(hash));
    return;
  }
  if (cached) {
    if (cached->page && !page(r.channel, *cached, r.session))
      return;
    if (!strcmp(c.operation, "get_state"))
      state(r.channel, r.session);
    send_payload(
        r.channel, "ack",
        module_ack_payload(*cached, cached->error ? "rejected" : "applied"),
        r.session);
    return;
  }
  error = modules.transitioning() ? "busy" : nullptr;
  if (error) {
    result.error = error;
    gate.remember(c, hash, result);
    send_payload(r.channel, "ack", module_ack_payload(result, "rejected"),
                 r.session);
    return;
  }
  send_payload(r.channel, "ack", module_ack_payload(result, "accepted"),
               r.session);
  if (!strcmp(c.operation, "set_mode")) {
    const char *mode = str(c.args, "mode");
    Profile target;
    if (!strcmp(mode, "sanitizador"))
      target = Profile::Sanitizador;
    else
      error = requested_profile(mode, &target);
    if (!error) {
      previous_profile = runtime.profile;
      error = modules.transition(target, monotonic_ms());
      if (!error && modules.transitioning()) {
        transition_command.pending = true;
        transition_command.result = result;
        transition_command.channel = r.channel;
        transition_command.session = r.session;
        memcpy(transition_command.digest, hash, 32);
        wipe(hash, sizeof(hash));
        return;
      }
      if (!error) {
        strcpy(runtime.last_command, c.id);
        event("system.profile_changed");
      } else
        event("system.error", error);
    }
  } else if (!strcmp(c.operation, "configure_connectivity")) {
    error = network.configure(c.args);
    if (!error) {
      if (runtime.profile == Profile::Cebo && !network.active() &&
          !network.start_cebo())
        error = "resource_error";
      if (!error) {
        ++runtime.revision;
        strcpy(runtime.last_command, c.id);
      }
    }
  } else if (!strcmp(c.operation, "set_time")) {
    const char *s = str(c.args, "timestamp");
    int year = atoi(s), month = atoi(s + 5), day = atoi(s + 8);
    auto leaps = [](int y) {
      --y;
      return y / 4 - y / 100 + y / 400;
    };
    int64_t days = 365LL * (year - 1970) + leaps(year) - leaps(1970) + day - 1;
    int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
      lengths[1] = 29;
    for (int m = 1; m < month; ++m)
      days += lengths[m - 1];
    timeval tv = {};
    tv.tv_sec =
        days * 86400 + atoi(s + 11) * 3600 + atoi(s + 14) * 60 + atoi(s + 17);
    if (s[19] == '.') {
      unsigned digits = 0;
      for (size_t i = 20; s[i] != 'Z' && digits < 6; ++i, ++digits)
        tv.tv_usec = tv.tv_usec * 10 + s[i] - '0';
      while (digits++ < 6)
        tv.tv_usec *= 10;
    }
    if (settimeofday(&tv, nullptr))
      error = "resource_error";
    else {
      has_time = true;
      ++runtime.revision;
      strcpy(runtime.last_command, c.id);
    }
  } else if (!strcmp(c.operation, "get_state")) {
    state(r.channel, r.session);
    result.state = true;
  } else if (!strcmp(c.operation, "read_events")) {
    uint64_t cursors[8];
    size_t n = 0;
    result = journal.page(number(c.args, "after_cursor"),
                          number(c.args, "limit"), cursors, &n);
    strcpy(result.id, c.id);
    if (!store.ready)
      error = "resource_error";
    for (size_t i = 0; !error && i < n; ++i) {
      char json[MaxMessage + 1];
      size_t len = sizeof(json);
      if (!journal.read(cursors[i], json, &len))
        error = "resource_error";
      else if (!send_raw(r.channel, json, len, r.session, "events"))
        error = "busy";
    }
  } else {
    ModuleReply reply;
    error = modules.dispatch(c, reply);
    if (!error) {
      if (reply.job)
        snprintf(result.body, sizeof(result.body),
                 "{\"kind\":\"job\",\"job_id\":\"%s\"}", c.id);
      else
        snprintf(result.body, sizeof(result.body), "%s", reply.result);
      if (strcmp(c.operation, "escudo.networks") &&
          strcmp(c.operation, "sanitizador.clients") &&
          strcmp(c.operation, "sanitizador.rules.list") &&
          strcmp(c.operation, "sanitizador.audit_result") &&
          strcmp(c.operation, "cebo.stats"))
        ++runtime.revision;
      strcpy(runtime.last_command, c.id);
    }
  }
  result.error = error;
  result.revision = runtime.revision;
  gate.remember(c, hash, result);
  wipe(hash, sizeof(hash));
  update_snapshot();
  send_payload(r.channel, "ack",
               module_ack_payload(result, error ? "rejected" : "applied"),
               r.session);
  publish_state();
}
static bool identity() {
  if (nvs_flash_init() != ESP_OK)
    return false; // Never erase identity/bonds as an automatic recovery.
  nvs_handle_t nvs;
  if (nvs_open("identity", NVS_READWRITE, &nvs) != ESP_OK)
    return false;
  size_t n = sizeof(device_id);
  esp_err_t err = nvs_get_str(nvs, "device_id", device_id, &n);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "node-%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    err = nvs_set_str(nvs, "device_id", device_id);
    if (err == ESP_OK)
      err = nvs_commit(nvs);
  }
  nvs_close(nvs);
  return err == ESP_OK && id_valid(device_id);
}
static void supervisor(void *) {
  Button button;
  uint64_t last_state = 0, last_metrics = 0, replay = 0, last_screen = 0;
  bool was_online = false;
  for (;;) {
    BusyResult rejected;
    if (xQueueReceive(busy_results, &rejected, 0) == pdTRUE) {
      rejected.result.revision = runtime.revision;
      const Result *known = nullptr;
      if (rejected.channel == Channel::Ble && rejected.session != ble_session())
        continue;
      if (rejected.expiry <= monotonic_ms()) {
        rejected.result.error = "expired";
        rejected.status = "rejected";
        rejected.cache = rejected.lookup_only = false;
      }
      if (rejected.expiry > monotonic_ms()) {
        Command c;
        strcpy(c.operation, rejected.operation);
        if (auto denied = v2::admission(c, rejected.channel, runtime.profile,
                                        true, false)) {
          rejected.result.error = denied;
          rejected.status = "rejected";
          rejected.cache = rejected.lookup_only = false;
        }
      }
      if ((rejected.cache || rejected.lookup_only) &&
          transition_command.pending &&
          !strcmp(rejected.result.id, transition_command.result.id)) {
        rejected.result.error =
            memcmp(rejected.digest, transition_command.digest, 32)
                ? "id_conflict"
                : nullptr;
        rejected.status = rejected.result.error ? "rejected" : "accepted";
        rejected.cache = rejected.lookup_only = false;
      }
      if (rejected.cache || rejected.lookup_only) {
        const char *conflict =
            gate.lookup(rejected.result.id, rejected.digest, &known);
        if (conflict)
          rejected.result.error = conflict;
        else if (!known && rejected.cache)
          gate.remember_id(rejected.result.id, rejected.digest,
                           rejected.result);
      }
      if (known) {
        bool sent =
            !known->page || page(rejected.channel, *known, rejected.session);
        if (known->state)
          state(rejected.channel, rejected.session);
        if (sent)
          send_payload(
              rejected.channel, "ack",
              module_ack_payload(*known, known->error ? "rejected" : "applied"),
              rejected.session);
      } else
        send_payload(rejected.channel, "ack",
                     module_ack_payload(rejected.result, rejected.status),
                     rejected.session);
    }
    Request r;
    if (xQueueReceive(requests, &r, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (r.command) {
        handle(r);
        auto &s = slots[r.slot];
        wipe(s.bytes, sizeof(s.bytes));
        s.size = 0;
        portENTER_CRITICAL(&pool_lock);
        pending.release(r.slot);
        portEXIT_CRITICAL(&pool_lock);
      } else
        switch (r.signal.kind) {
        case Kind::ShortPress: {
          if (modules.transitioning())
            break;
          auto target = modules.next();
          previous_profile = runtime.profile;
          const char *e = modules.transition(target, monotonic_ms());
          button_transition_pending = modules.transitioning();
          if (!button_transition_pending)
            event(e ? "system.error" : "system.profile_changed", e);
          update_snapshot();
          state(Channel::Ble);
          publish_state();
          break;
        }
        case Kind::PairWindow:
          if (has_oled) {
            ble_window(monotonic_ms() + 120000);
            passkey_until = 0;
          }
          break;
        case Kind::Passkey: {
          snprintf(shown_passkey, sizeof(shown_passkey), "%06lu",
                   static_cast<unsigned long>(r.signal.value));
          has_oled = oled_show("PAIR CODE", shown_passkey, "ANDROID PASSKEY",
                               "120S WINDOW");
          passkey_until = ble_deadline();
          ble_passkey(r.signal.connection, r.signal.value, has_oled);
          if (!has_oled)
            ble_window(0);
          break;
        }
        case Kind::BleDown:
          passkey_until = 0;
          wipe(shown_passkey, sizeof(shown_passkey));
          break;
        case Kind::BleUp:
          break;
        case Kind::WifiUp:
          network.wifi_up();
          break;
        case Kind::WifiDown:
          network.wifi_down();
          break;
        case Kind::MqttUp:
          network.mqtt_up();
          break;
        case Kind::MqttDown:
          network.mqtt_down();
          break;
        case Kind::MqttError:
          network.mqtt_down(true);
          runtime.error = "resource_error";
          break;
        }
    }
    uint64_t now = monotonic_ms();
    if (has_button) {
      int action = button.update(button_down(), now);
      if (action && !post({action == 2 ? Kind::PairWindow : Kind::ShortPress}))
        runtime.error = "busy";
    }
    modules.tick(now);
    if (button_transition_pending && !modules.transitioning()) {
      button_transition_pending = false;
      event(runtime.error ? "system.error" : "system.profile_changed",
            runtime.error);
      state(Channel::Ble);
      publish_state();
    }
    if (transition_command.pending && !modules.transitioning()) {
      auto &t = transition_command;
      t.result.error = runtime.error;
      t.result.revision = runtime.revision;
      if (!runtime.error)
        strcpy(runtime.last_command, t.result.id);
      event(runtime.error ? "system.error" : "system.profile_changed",
            runtime.error);
      gate.remember_id(t.result.id, t.digest, t.result);
      send_payload(
          t.channel, "ack",
          module_ack_payload(t.result, t.result.error ? "rejected" : "applied"),
          t.session);
      wipe(t.digest, sizeof(t.digest));
      t.pending = false;
      state(Channel::Ble);
      publish_state();
    }
    ModuleOutput output;
    if (module_outputs.pop(output)) {
      auto payload =
          output.metric
              ? cJSON_CreateObject()
              : module_event_payload(journal.highest + 1, output.name,
                                     output.mode, nullptr, output.json,
                                     output.severity, output.simulated);
      if (output.metric) {
        cJSON_AddStringToObject(payload, "scope", profile_name(output.mode));
        cJSON_AddNumberToObject(payload, "interval_ms", output.interval_ms);
        cJSON_AddItemToObject(payload, "values",
                              v2::parse(output.json, strlen(output.json)));
      }
      char timestamp[33], json[MaxMessage + 1];
      if (serialize(envelope(context(timestamp),
                             output.metric ? "metrics" : "event", payload),
                    json)) {
        if (output.metric ||
            journal.append(json, strlen(json), journal.highest + 1)) {
          ble_send(json, strlen(json));
          network.publish(output.metric ? "metrics" : "events", json,
                          strlen(json), output.metric ? 0 : 1, false);
        } else
          runtime.error = "resource_error";
      } else
        runtime.error = "invalid_message";
    }
    network.tick(now, has_time);
    update_snapshot();
    bool online = network.connected();
    if (online && !was_online) {
      replay = journal.first();
      publish_state();
      last_state = now;
    }
    was_online = online;
    if (online && now - last_state >= 5000) {
      publish_state();
      last_state = now;
    }
    if (online && replay && replay <= journal.highest) {
      char json[MaxMessage + 1];
      size_t n = sizeof(json);
      if (journal.read(replay, json, &n)) {
        if (network.publish("events", json, n, 1, false))
          ++replay;
      } else
        ++replay;
    }
    if ((online || !strcmp(ble_status(), "connected")) &&
        now - last_metrics >= 5000) {
      auto p = cJSON_CreateObject();
      cJSON_AddStringToObject(p, "scope", "system");
      cJSON_AddNumberToObject(p, "interval_ms", 5000);
      auto values = cJSON_AddObjectToObject(p, "values");
      cJSON_AddNumberToObject(values, "free_heap_bytes",
                              esp_get_free_heap_size());
      cJSON_AddNumberToObject(values, "min_free_heap_bytes",
                              esp_get_minimum_free_heap_size());
      cJSON_AddNumberToObject(values, "journal_count", journal.count());
      cJSON_AddNumberToObject(values, "queue_count",
                              uxQueueMessagesWaiting(requests));
      cJSON_AddNumberToObject(values, "dropped_messages",
                              dropped_messages.load() + network.dropped());
      cJSON_AddNumberToObject(values, "dropped_events",
                              module_outputs.dropped());
      char timestamp[33], json[MaxMessage + 1];
      if (serialize(envelope(context(timestamp), "metrics", p), json)) {
        ble_send(json, strlen(json));
        network.publish("metrics", json, strlen(json), 0, false);
      }
      last_metrics = now;
    }
    if (has_oled && now - last_screen >= 500 && now >= passkey_until) {
      char connectivity[48];
      auto short_status = [](const char *s) {
        return !strcmp(s, "connected")    ? "UP"
               : !strcmp(s, "connecting") ? "TRY"
               : !strcmp(s, "error")      ? "ERR"
                                          : "DOWN";
      };
      snprintf(connectivity, sizeof(connectivity), "W:%s M:%s",
               short_status(network.wifi_state),
               short_status(network.mqtt_state));
      has_oled =
          oled_show(profile_name(runtime.profile), runtime.phase, connectivity,
                    runtime.error ? runtime.error
                                  : (now < ble_deadline() ? "PAIR WINDOW"
                                                          : "BLE CONTROL"));
      if (!has_oled) {
        ble_window(0);
        runtime.error = "resource_error";
      }
      last_screen = now;
    }
  }
}
void start() {
  requests = xQueueCreate(QueueSize, sizeof(Request));
  busy_results = xQueueCreate(QueueSize, sizeof(BusyResult));
  snapshot_lock = xSemaphoreCreateMutex();
  if (!requests || !busy_results || !snapshot_lock) {
    ESP_LOGE("sentinel", "resource_error: queue allocation");
    return;
  }
  uint32_t random[4];
  esp_fill_random(random, sizeof(random));
  snprintf(boot_id, sizeof(boot_id), "boot-%08lx%08lx%08lx%08lx",
           static_cast<unsigned long>(random[0]),
           static_cast<unsigned long>(random[1]),
           static_cast<unsigned long>(random[2]),
           static_cast<unsigned long>(random[3]));
  bool identity_ok = identity();
  if (!identity_ok) {
    runtime.healthy = false;
    runtime.phase = "error";
    runtime.error = "resource_error";
    ESP_LOGE("sentinel", "resource_error: identity NVS; no erase performed");
    return;
  }
  uint32_t flash = 0;
  esp_err_t flash_status = esp_flash_get_size(nullptr, &flash);
  size_t psram = esp_psram_get_size();
  if (flash_status != ESP_OK || flash != 16 * 1024 * 1024 ||
      psram != 8 * 1024 * 1024) {
    runtime.healthy = false;
    runtime.phase = "error";
    runtime.error = "resource_error";
  }
  printf("Sentinel %s device=%s boot=%s flash=%lu psram=%u profile=idle "
         "revision=0\n",
         SENTINEL_VERSION, device_id, boot_id,
         static_cast<unsigned long>(flash), static_cast<unsigned>(psram));
  has_oled = oled_init();
  has_button = button_init();
  if (!has_oled) {
    runtime.error = "resource_error";
    ESP_LOGE("sentinel",
             "resource_error: OLED unavailable; new pairing disabled");
  }
  if (!store.init()) {
    journal.degraded = true;
    runtime.error = "resource_error";
  } else
    journal.recover();
  if (!network.init(device_id)) {
    runtime.healthy = false;
    runtime.phase = "error";
    runtime.error = "resource_error";
  }
  registration_open = true;
  register_optional_modules();
  registration_open = false;
  update_snapshot();
  ble_init();
  event("system.booted");
  xTaskCreate(supervisor, "supervisor", 24576, nullptr, 5, nullptr);
}
} // namespace sentinel
