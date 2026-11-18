#include "../SentinelNode/src/sentinel/include/network.h"
#include "../SentinelNode/src/sentinel/include/module_runtime.h"
#include "../SentinelNode/src/sentinel/include/sentinel.h"
#include "http_parser.h"
#include "nvs.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>

namespace {
std::vector<unsigned char> committed, pending;
bool namespace_exists = false, fail_open = false, fail_read = false,
     fail_write = false, fail_commit = false, short_read = false;
unsigned writes = 0, commits = 0, starts = 0, connects = 0, stops = 0,
         mqtt_starts = 0;
wifi_mode_t mode = WIFI_MODE_NULL;
wifi_config_t applied;
bool mqtt_stopped = true;
esp_netif_t netif;
void reset_io() { fail_open = fail_read = fail_write = fail_commit = short_read = false; }
std::string read(const char *path) {
  std::ifstream stream(path); assert(stream);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
cJSON *args(const char *ssid = "own-lab") {
  auto j = cJSON_CreateObject();
  for (auto pair : {std::pair{"ssid", ssid}, {"wifi_password", "fixture-wifi"},
       {"mqtt_uri", "mqtt://broker.example:1883"}, {"mqtt_username", "fixture-user"},
       {"mqtt_password", "fixture-mqtt"}})
    cJSON_AddStringToObject(j, pair.first, pair.second);
  return j;
}
void replace(cJSON *j, const char *key, const std::string &value) {
  assert(cJSON_ReplaceItemInObjectCaseSensitive(j, key, cJSON_CreateString(value.c_str())));
}
void expect_error(const char *error, const char *expected) {
  assert(error && !strcmp(error, expected));
}
// Lifecycle fixtures only; no Cebo/Sanitizador services are implemented here.
struct Context : sentinel::ModuleContext {
  sentinel::Network &network;
  explicit Context(sentinel::Network &n) : network(n) {}
  uint64_t now_ms() const override { return 100; }
  sentinel::ModuleResources &resources() override { return network; }
  bool emit_event(const char *, const char *, const char *, bool) override { return true; }
  bool publish_metrics(uint32_t, const char *) override { return true; }
};
struct LifecycleFixture : sentinel::Module {
  std::string snapshot_json;
  explicit LifecycleFixture(const char *fixture) {
    auto document = cJSON_Parse(read(fixture).c_str()); assert(document);
    auto module = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(document, "payload"), "module");
    auto json = cJSON_PrintUnformatted(module); assert(json);
    snapshot_json = json; cJSON_free(json); cJSON_Delete(document);
  }
  bool init(sentinel::ModuleContext &) override { return true; }
  bool start() override { return true; }
  void request_stop() override {}
  bool stopped() const override { return true; }
  void handle(const sentinel::ModuleCommand &, sentinel::ModuleReply &) override {}
  void tick(uint64_t) override {}
  void snapshot(sentinel::ModuleSnapshot &out) const override { strcpy(out.state, snapshot_json.c_str()); }
};
}

int nvs_open(const char *name, int access, nvs_handle_t *out) {
  assert(!strcmp(name, "connectivity")); // Cannot touch bonds/identity/journal.
  if (fail_open) return ESP_FAIL;
  if (!namespace_exists && access == NVS_READONLY) return ESP_ERR_NVS_NOT_FOUND;
  namespace_exists = true; *out = 1; return ESP_OK;
}
void nvs_close(nvs_handle_t h) { assert(h == 1); pending.clear(); }
int nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size) {
  assert(h == 1 && !strcmp(key, "config"));
  if (fail_read) return ESP_FAIL;
  if (committed.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if (!out) { *size = committed.size(); return ESP_OK; }
  if (*size < committed.size()) return ESP_ERR_NVS_INVALID_LENGTH;
  *size = committed.size() - (short_read ? 1 : 0);
  memcpy(out, committed.data(), *size); return ESP_OK;
}
int nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t size) {
  assert(h == 1 && !strcmp(key, "config")); ++writes;
  if (fail_write) return ESP_FAIL;
  pending.assign(static_cast<const unsigned char *>(data),
                 static_cast<const unsigned char *>(data) + size);
  return ESP_OK;
}
int nvs_commit(nvs_handle_t h) {
  assert(h == 1); ++commits;
  if (fail_commit) return ESP_FAIL;
  committed = pending; return ESP_OK;
}
esp_event_base_t WIFI_EVENT = "wifi", IP_EVENT = "ip";
QueueHandle_t xQueueCreate(unsigned, unsigned) { return reinterpret_cast<QueueHandle_t>(1); }
int xQueueSend(QueueHandle_t, const void *, unsigned) { return pdTRUE; }
int xQueueReceive(QueueHandle_t, void *, unsigned) { return 0; }
void xQueueReset(QueueHandle_t) {}
int esp_netif_init() { return ESP_OK; }
int esp_event_loop_create_default() { return ESP_OK; }
esp_netif_t *esp_netif_create_default_wifi_sta() { return &netif; }
esp_netif_t *esp_netif_create_default_wifi_ap() { return &netif; }
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *) { return &netif; }
int esp_event_handler_register(esp_event_base_t, int, EventHandler, void *) { return ESP_OK; }
int esp_wifi_init(const wifi_init_config_t *cfg) { assert(!cfg->nvs_enable); return ESP_OK; }
int esp_wifi_set_storage(int value) { assert(value == WIFI_STORAGE_RAM); return ESP_OK; }
int esp_wifi_set_mode(wifi_mode_t value) { mode = value; return ESP_OK; }
int esp_wifi_get_mode(wifi_mode_t *value) { *value = mode; return ESP_OK; }
int esp_wifi_set_config(int, wifi_config_t *value) { applied = *value; return ESP_OK; }
int esp_wifi_start() { ++starts; return ESP_OK; }
int esp_wifi_stop() { ++stops; return ESP_OK; }
int esp_wifi_connect() { ++connects; return ESP_OK; }
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *) { return ESP_OK; }
int esp_wifi_set_promiscuous(bool) { return ESP_OK; }
int esp_wifi_set_channel(uint8_t, int) { return ESP_OK; }
int esp_wifi_scan_start(wifi_scan_config_t *, bool) { return ESP_OK; }
int esp_wifi_scan_get_ap_records(uint16_t *, wifi_ap_record_t *) { return ESP_OK; }
int esp_wifi_clear_ap_list() { return ESP_OK; }
int esp_wifi_get_country(wifi_country_t *) { return ESP_OK; }
int esp_wifi_set_promiscuous_rx_cb(void (*)(void *, wifi_promiscuous_pkt_type_t)) { return ESP_OK; }
int esp_wifi_set_promiscuous_filter(wifi_promiscuous_filter_t *) { return ESP_OK; }
int esp_netif_dhcps_stop(esp_netif_t *) { return ESP_OK; }
int esp_netif_dhcps_start(esp_netif_t *) { return ESP_OK; }
int esp_netif_dhcps_get_status(esp_netif_t *, esp_netif_dhcp_status_t *status) {
  *status = ESP_NETIF_DHCP_STOPPED; return ESP_OK;
}
int esp_netif_get_ip_info(esp_netif_t *, esp_netif_ip_info_t *info) { info->ip.addr = 1; return ESP_OK; }
int esp_netif_set_ip_info(esp_netif_t *, esp_netif_ip_info_t *) { return ESP_OK; }
int esp_netif_napt_enable(esp_netif_t *) { return ESP_OK; }
int esp_netif_napt_disable(esp_netif_t *) { return ESP_OK; }
int http_parser_parse_url(const char *uri, size_t, int, http_parser_url *out) {
  assert(!strcmp(uri, "mqtt://broker.example:1883"));
  out->field_set = 3; out->port = 1883;
  out->field_data[UF_HOST] = {7, 14}; return 0;
}
namespace sentinel {
uint64_t monotonic_ms() { return 100; }
bool post(Signal) { return true; }
bool submit(Channel, const char *, size_t, bool, bool, uint32_t) { return true; }
bool MqttTransport::init(StateCallback, MessageCallback) { return true; }
bool MqttTransport::start(const Config &cfg) {
  assert(cfg.port == 1883 && !strcmp(cfg.user, "fixture-user") &&
         !strcmp(cfg.password, "fixture-mqtt") && !strcmp(cfg.host, "broker.example"));
  ++mqtt_starts; mqtt_stopped = false; return true;
}
void MqttTransport::request_stop() { mqtt_stopped = true; }
bool MqttTransport::stopped() const { return mqtt_stopped; }
bool MqttTransport::stopping() const { return !mqtt_stopped; }
bool MqttTransport::connected() const { return false; }
bool MqttTransport::enqueue(const char *, const char *, size_t, int, bool) { return true; }
}

int main() {
  using namespace sentinel;
  ConnectivityConfig restored;
  assert(load_connectivity(restored) == ConnectivityLoad::Missing);
  namespace_exists = true; // Existing namespace but no key is also normal.
  assert(load_connectivity(restored) == ConnectivityLoad::Missing);
  auto original = args(), replacement = args("own-new-lab");
  Network network;
  assert(network.init("fixture-node") && !network.configuration_error());
  Runtime state;
  ModuleSupervisor supervisor(state, network);
  Context context(network);
  LifecycleFixture cebo("contracts/fixtures/v2/states/cebo.valid.json"),
      escudo("contracts/fixtures/v2/states/escudo.valid.json");
  assert(supervisor.add(Profile::Cebo, cebo, context));
  assert(supervisor.add(Profile::Escudo, escudo, context));
  assert(state.profile == Profile::Idle && mode == WIFI_MODE_NULL && starts == 0);
  assert(!network.configure(original));
  assert(commits == 1 && !network.active());
  const auto valid_blob = committed;
  assert(load_connectivity(restored) == ConnectivityLoad::Loaded);
  assert(!strcmp(restored.ssid, "own-lab"));
  assert(!supervisor.transition(Profile::Cebo, 100));
  assert(network.active() && !memcmp(applied.sta.ssid, "own-lab", 7));
  for (bool *failure : {&fail_open, &fail_write, &fail_commit}) {
    auto before_stops = stops, before_starts = starts;
    *failure = true;
    expect_error(network.configure(replacement), "resource_error");
    assert(network.configuration_error() && network.active());
    assert(stops == before_stops && starts == before_starts && committed == valid_blob);
    assert(!memcmp(applied.sta.ssid, "own-lab", 7));
    reset_io();
    assert(!supervisor.transition(Profile::Idle, 101));
    assert(!supervisor.transition(Profile::Cebo, 102));
    assert(!memcmp(applied.sta.ssid, "own-lab", 7)); // Previous RAM config survives.
  }
  mqtt_stopped = false;
  auto before_writes = writes;
  expect_error(network.configure(replacement), "busy");
  assert(writes == before_writes);
  mqtt_stopped = true;
  assert(!network.configure(replacement) && !network.configuration_error());
  assert(!memcmp(applied.sta.ssid, "own-new-lab", 11));
  assert(!supervisor.transition(Profile::Idle, 103));
  assert(!supervisor.transition(Profile::Escudo, 104));
  before_writes = writes;
  expect_error(network.configure(original), "invalid_state");
  auto before_connects = connects;
  network.tick(10000, true);
  assert(connects == before_connects && writes == before_writes && mqtt_starts == 0);
  assert(!supervisor.transition(Profile::Idle, 105));

  // Recovery uses the production Network::init, not a storage-only stand-in.
  committed = valid_blob;
  Network reboot;
  before_connects = connects;
  auto before_starts = starts;
  assert(reboot.init("fixture-node") && !reboot.configuration_error());
  Runtime reboot_state;
  ModuleSupervisor reboot_supervisor(reboot_state, reboot);
  Context reboot_context(reboot);
  assert(reboot_supervisor.add(Profile::Cebo, cebo, reboot_context));
  reboot.tick(50000, true);
  assert(reboot_state.profile == Profile::Idle && !reboot.active());
  assert(mode == WIFI_MODE_NULL && starts == before_starts && connects == before_connects);
  assert(!reboot_supervisor.transition(Profile::Cebo, 106));
  reboot.tick(50001, false); // MQTT connects with unknown UTC after restart.
  assert(mqtt_starts == 1 && !strcmp(reboot.mqtt_state, "connecting"));
  reboot.tick(50002, true);
  assert(mqtt_starts == 1);
  assert(!reboot_supervisor.transition(Profile::Idle, 107));

  for (unsigned fault = 0; fault < 9; ++fault) {
    reset_io(); committed = valid_blob;
    if (fault == 0) committed.pop_back();
    if (fault == 1) committed.push_back(0);
    if (fault == 2) committed[0] ^= 1;
    if (fault == 3) committed[4] = 1; // Unsupported format, no migration.
    if (fault == 4) std::fill(committed.begin() + 8, committed.begin() + 41, 'x'); // No NUL.
    if (fault == 5) committed[8] = 0; // Empty SSID.
    if (fault == 6) fail_open = true;
    if (fault == 7) fail_read = true;
    if (fault == 8) short_read = true;
    auto damaged = committed;
    before_writes = writes; before_starts = starts;
    assert(load_connectivity(restored) == ConnectivityLoad::Error);
    assert(!strcmp(restored.ssid, "own-lab")); // Do not expose partial reads.
    Network broken;
    assert(broken.init("fixture-node") && broken.configuration_error());
    assert(!broken.active() && starts == before_starts && writes == before_writes);
    assert(broken.start_profile(Profile::Cebo) && !broken.active());
    assert(committed == damaged);
  }
  reset_io(); committed = valid_blob;
  Network validator;
  assert(validator.init("fixture-node"));
  for (auto test : {std::pair{"ssid", std::string(33, 's')},
       {"wifi_password", std::string(64, 'w')}, {"mqtt_username", std::string(65, 'u')},
       {"mqtt_password", std::string(129, 'p')}, {"mqtt_uri", std::string("mqtts://broker.example")},
       {"mqtt_username", std::string("\xc0\xaf")}}) {
    auto invalid = args(); replace(invalid, test.first, test.second);
    before_writes = writes;
    expect_error(validator.configure(invalid), "invalid_message");
    assert(writes == before_writes && committed == valid_blob);
    cJSON_Delete(invalid);
  }
  // An old MQTT/TLS command must be rejected without touching current storage.
  auto legacy = args();
  cJSON_AddStringToObject(legacy, "ca_certificate", "obsolete-public-ca");
  before_writes = writes;
  expect_error(validator.configure(legacy), "invalid_message");
  assert(writes == before_writes && committed == valid_blob);
  cJSON_Delete(legacy);
  // Even correctly-sized blobs must pass content validation on restore.
  ConnectivityConfig invalid;
  assert(invalid.from_args(original));
  for (auto target : {invalid.uri, invalid.username}) {
    assert(invalid.from_args(original));
    target[0] = '\xff';
    memcpy(committed.data() + 8, &invalid, sizeof(invalid));
    assert(load_connectivity(restored) == ConnectivityLoad::Error);
  }
  for (auto content : {std::pair{"mqtt_uri", std::string("mqtts://broker.example")},
       {"mqtt_username", std::string(65, 'u')}}) {
    // Build a correctly sized but semantically corrupt blob, bypassing save validation.
    assert(invalid.from_args(original));
    char *destination = !strcmp(content.first, "mqtt_uri") ? invalid.uri
        : invalid.username;
    strcpy(destination, content.second.c_str());
    memcpy(committed.data() + 8, &invalid, sizeof(invalid));
    assert(load_connectivity(restored) == ConnectivityLoad::Error);
  }
  alignas(ConnectivityConfig) unsigned char temporary[sizeof(ConnectivityConfig)];
  auto sensitive = new (temporary) ConnectivityConfig;
  assert(sensitive->from_args(original));
  sensitive->~ConnectivityConfig();
  assert(std::all_of(std::begin(temporary), std::end(temporary), [](auto byte) { return byte == 0; }));
  cJSON_Delete(original); cJSON_Delete(replacement);
  std::cout << "Production connectivity NVS + Network: save/restore/errors/rollback/Idle/Escudo/UTC/privacy PASS (host doubles)\n";
}
