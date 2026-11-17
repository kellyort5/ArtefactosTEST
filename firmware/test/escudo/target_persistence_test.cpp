#include "test_support.hpp"
#include "frame_fixtures.hpp"
#include "escudo_target.h"
#include "nvs.h"
#include <algorithm>

// Only the SDK boundary is doubled. Every selection, baseline, frame parser,
// detector, supervisor, ACK and NVS-helper operation below uses real sources.
namespace sdk {
enum class Fail { None, OpenRead, OpenWrite, Size, Read, Write, Commit };
struct Storage {
  Fail fail = Fail::None;
  std::vector<uint8_t> committed, pending;
  unsigned opens = 0, closes = 0, writes = 0, commits = 0, reads = 0;
  int mode = NVS_READONLY;
  bool present = false, wrong_read_size = false;
  void (*on_commit)() = nullptr;
} storage;
} // namespace sdk
using sdk::Fail;
using sdk::storage;
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle) {
  assert(!strcmp(name, "escudo_target")); // No foreign namespace access.
  ++storage.opens;
  if ((mode == NVS_READONLY && storage.fail == Fail::OpenRead) ||
      (mode == NVS_READWRITE && storage.fail == Fail::OpenWrite))
    return ESP_FAIL;
  if (mode == NVS_READONLY && !storage.present)
    return ESP_ERR_NVS_NOT_FOUND;
  storage.mode = mode;
  storage.pending.clear();
  *handle = 7;
  return ESP_OK;
}
void nvs_close(nvs_handle_t h) {
  assert(h == 7);
  ++storage.closes;
  storage.pending.clear();
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size) {
  assert(h == 7 && !strcmp(key, "selection"));
  ++storage.reads;
  if (storage.committed.empty())
    return ESP_ERR_NVS_NOT_FOUND;
  if ((!out && storage.fail == Fail::Size) ||
      (out && storage.fail == Fail::Read))
    return ESP_FAIL;
  if (out) {
    assert(*size >= storage.committed.size());
    memcpy(out, storage.committed.data(), storage.committed.size());
  }
  *size = storage.committed.size() + (out && storage.wrong_read_size ? 1 : 0);
  return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data,
                       size_t size) {
  assert(h == 7 && storage.mode == NVS_READWRITE &&
         !strcmp(key, "selection") && size == 51);
  ++storage.writes;
  if (storage.fail == Fail::Write)
    return ESP_FAIL;
  auto p = static_cast<const uint8_t *>(data);
  storage.pending.assign(p, p + size);
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
  assert(h == 7 && storage.mode == NVS_READWRITE && !storage.pending.empty());
  ++storage.commits;
  if (storage.on_commit)
    storage.on_commit();
  if (storage.fail == Fail::Commit)
    return ESP_FAIL;
  storage.committed = storage.pending;
  storage.present = true;
  return ESP_OK;
}
// The SDK double intentionally exposes no erase, format or flash-init API.
static constexpr const char *First =
    "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}";
static constexpr const char *Second =
    "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:02\",\"channel\":11}";
static void snapshot(Harness &h, unsigned id, const char *capture) {
  ModuleSnapshot s;
  h.module.snapshot(s);
  auto j = cJSON_Parse(s.state);
  assert(j && v2::validate("V2EscudoModule", j));
  auto target = cJSON_GetObjectItemCaseSensitive(j, "target");
  assert(s.target_selected == bool(id));
  if (id) {
    char bssid[18];
    snprintf(bssid, sizeof(bssid), "02:00:00:00:00:%02x", id & 255);
    assert(!strcmp(text(target, "bssid"), bssid));
    assert(!strcmp(text(target, "ssid"), "LAB"));
    assert(number(target, "channel") == (id == 1 ? 6 : 11));
  } else
    assert(cJSON_IsNull(target));
  assert(!strcmp(text(j, "capture_status"), capture));
  cJSON_Delete(j);
}
static void scan(Harness &h) {
  assert(!h.command("escudo.scan").error);
  h.radio.records = {network(1), network(2, "LAB", 11)};
  h.radio.done();
  h.radio.events.back().uptime_ms = h.context.now;
  h.tick();
}
static void choose(Harness &h) {
  scan(h);
  assert(!h.command("escudo.select_target", First).error);
  h.tick();
}
static void rejected(const Result &r) {
  assert(r.error && !strcmp(r.error, "resource_error"));
}
static unsigned detections(const Harness &h, const char *name = "escudo.detection") {
  return std::count_if(h.context.events.begin(), h.context.events.end(),
                       [name](const std::string &s) {
                         return s.find(name) != std::string::npos;
                       });
}
static void feed(Harness &h, const fixtures::Bytes &b, uint8_t channel = 6) {
  h.context.now += 10;
  h.radio.feed(fixtures::frame(b, 1, channel, h.context.now));
  h.tick();
}
static void helper_tests() {
  storage = {};
  ScanNetwork out = network(99);
  assert(load_escudo_target(out) == EscudoTargetLoad::Missing && out.bssid[5] == 99);
  storage.present = true; // Namespace exists but selection does not.
  assert(load_escudo_target(out) == EscudoTargetLoad::Missing);
  for (int kind = 0; kind < 4; ++kind) {
    auto n = network(1);
    if (kind == 1)
      memset(n.ssid, 0, sizeof(n.ssid)); // hidden
    if (kind == 2)
      n.ssid[0] = char(0xff); // binary SSID supported by the wire codec
    if (kind == 3) {
      memset(n.ssid, 'x', 32);
      n.ssid[32] = 0;
      n.auth = 9; // OWE remains unknown, never becomes open.
      n.pairwise_cipher = n.group_cipher = 12;
    }
    assert(save_escudo_target(n));
    assert(storage.committed.size() == 51 && storage.pending.empty());
    assert(load_escudo_target(out) == EscudoTargetLoad::Loaded);
    assert(!memcmp(out.ssid, n.ssid, sizeof(n.ssid)) &&
           !memcmp(out.bssid, n.bssid, 6) && out.channel == n.channel &&
           out.auth == n.auth && out.pairwise_cipher == n.pairwise_cipher &&
           out.group_cipher == n.group_cipher && out.rssi == 0);
  }
  const auto golden = storage.committed;
  for (auto failure : {Fail::OpenWrite, Fail::Write, Fail::Commit}) {
    storage.fail = failure;
    assert(!save_escudo_target(network(2)) && storage.committed == golden);
    assert(storage.pending.empty());
  }
  storage.fail = Fail::None;
  for (unsigned kind = 0; kind < 8; ++kind) {
    auto n = network(1);
    switch (kind) {
    case 0: memset(n.ssid, 'x', sizeof(n.ssid)); break;
    case 1: memset(n.bssid, 0, 6); break;
    case 2: n.bssid[0] = 1; break;
    case 3: n.channel = 0; break;
    case 4: n.channel = 15; break;
    case 5: n.auth = 17; break;
    case 6: n.pairwise_cipher = 13; break;
    case 7: n.group_cipher = 13; break;
    }
    auto opens = storage.opens;
    assert(!save_escudo_target(n) && storage.opens == opens);
  }
}
static void invalid_storage_tests() {
  storage = {};
  assert(save_escudo_target(network(1)));
  const auto golden = storage.committed;
  std::vector<std::vector<uint8_t>> bad;
  for (size_t size : {size_t(1), size_t(50), size_t(52), size_t(4096)})
    bad.emplace_back(size, 0);
  // Header/version, unterminated SSID, zero/multicast BSSID, invalid channels,
  // and out-of-range pinned SDK security enums.
  for (unsigned kind = 0; kind < 10; ++kind) {
    auto b = golden;
    switch (kind) {
    case 0: b[0] ^= 1; break;
    case 1: b[4] = 2; break;
    case 2: std::fill(b.begin() + 8, b.begin() + 41, 'x'); break;
    case 3: std::fill(b.begin() + 41, b.begin() + 47, 0); break;
    case 4: b[41] |= 1; break;
    case 5: b[47] = 0; break;
    case 6: b[47] = 15; break;
    case 7: b[48] = 17; break;
    case 8: b[49] = 13; break;
    case 9: b[50] = 13; break;
    }
    bad.push_back(b);
  }
  for (auto &bytes : bad) {
    storage.committed = bytes;
    auto writes = storage.writes, commits = storage.commits;
    ScanNetwork out = network(99);
    assert(load_escudo_target(out) == EscudoTargetLoad::Error && out.bssid[5] == 99);
    Harness h;
    snapshot(h, 0, "stopped");
    assert(!h.radio.scan_calls && !h.radio.channel_calls && !h.radio.capture_calls);
    h.tick();
    assert(detections(h, "resource_error") == 1 && !detections(h));
    assert(storage.committed == bytes && storage.writes == writes && storage.commits == commits);
    // A corrupt record never prevents an explicit valid replacement.
    choose(h);
    snapshot(h, 1, "stopped");
  }
  for (auto failure : {Fail::OpenRead, Fail::Size, Fail::Read}) {
    storage.committed = golden;
    storage.fail = failure;
    ScanNetwork out = network(99);
    assert(load_escudo_target(out) == EscudoTargetLoad::Error && out.bssid[5] == 99);
    Harness h;
    h.tick();
    snapshot(h, 0, "stopped");
    assert(detections(h, "resource_error") == 1);
  }
  storage.fail = Fail::None;
  storage.wrong_read_size = true;
  ScanNetwork out;
  assert(load_escudo_target(out) == EscudoTargetLoad::Error);
}
static Harness *committing = nullptr;
static void before_commit() {
  assert(committing);
  snapshot(*committing, 1, "running"); // Accepted state is still previous.
  assert(committing->radio.channel == 11 && committing->radio.physical_capture);
}
static void selection_failure_tests() {
  for (bool running : {false, true})
    for (auto failure : {Fail::None, Fail::OpenWrite, Fail::Write, Fail::Commit}) {
      storage = {};
      Harness h;
      choose(h);
      auto golden = storage.committed;
      if (running)
        assert(!h.command("escudo.capture_start").error);
      storage.fail = failure;
      committing = &h;
      storage.on_commit = running ? before_commit : nullptr;
      auto commits = storage.commits;
      auto result = h.command("escudo.select_target", Second);
      storage.on_commit = nullptr;
      if (failure == Fail::None) {
        assert(!result.error && storage.commits == commits + 1 && storage.committed != golden);
        snapshot(h, 2, running ? "running" : "stopped");
        ScanNetwork restored;
        assert(load_escudo_target(restored) == EscudoTargetLoad::Loaded &&
               restored.bssid[5] == 2 && restored.channel == 11);
      } else {
        rejected(result);
        snapshot(h, 1, "stopped");
        assert(!h.radio.physical_capture && storage.committed == golden);
        h.tick();
        assert(detections(h, "resource_error") >= 1);
        assert(detections(h, "escudo.target") == 1); // Only the earlier target event.
        storage.fail = Fail::None;
        assert(!h.command("escudo.select_target", Second).error);
        snapshot(h, 2, "stopped");
      }
      assert(!h.command("escudo.capture_stop").error);
    }
  // Fail the *new* capture release after its persistence fails. Invalidate its
  // generation, reject restart, and retry cleanup on tick without any auto-start.
  storage = {};
  Harness h;
  choose(h);
  auto golden = storage.committed;
  unsigned observed = 0;
  h.module.observe([](void *p, const Observation &) { ++*static_cast<unsigned *>(p); },
                   &observed);
  assert(!h.command("escudo.capture_start").error);
  committing = &h;
  storage.on_commit = [] {
    before_commit();
    committing->radio.feed(fixtures::frame(fixtures::beacon(false, 2, 11),
                                           1, 11, committing->context.now));
    committing->radio.capture_stop_ok = false;
  };
  storage.fail = Fail::Commit;
  rejected(h.command("escudo.select_target", Second));
  storage.on_commit = nullptr;
  snapshot(h, 1, "error");
  assert(h.radio.physical_capture && !h.radio.capture_generation());
  rejected(h.command("escudo.capture_start"));
  h.tick();
  snapshot(h, 1, "error");
  h.radio.capture_stop_ok = true;
  h.tick();
  snapshot(h, 1, "stopped");
  assert(!h.radio.physical_capture && storage.committed == golden && !observed);
  storage.fail = Fail::None;
  assert(!h.command("escudo.capture_start").error && h.radio.channel == 6);
  // Radio staging failures must never persist the rejected selection.
  for (unsigned kind = 0; kind < 3; ++kind) {
    if (kind == 0) h.radio.capture_stop_ok = false;
    if (kind == 1) h.radio.capture_ok = false;
    if (kind == 2) h.radio.channel_ok = false;
    auto writes = storage.writes;
    rejected(h.command("escudo.select_target", Second));
    assert(storage.writes == writes && storage.committed == golden);
    h.radio.capture_stop_ok = h.radio.capture_ok = h.radio.channel_ok = true;
    h.tick();
    assert(!h.command("escudo.capture_stop").error);
    snapshot(h, 1, "stopped");
    assert(!h.command("escudo.capture_start").error);
  }
  assert(!h.command("escudo.capture_stop").error);
}
static void restoration_tests() {
  storage = {};
  {
    Harness h;
    h.context.now = 900000;
    choose(h);
    assert(!h.command("escudo.capture_start").error);
    for (unsigned i = 0; i < 4; ++i) feed(h, fixtures::deauth());
    assert(!detections(h));
    auto writes = storage.writes;
    assert(!h.supervisor.transition(Mode::Idle, h.context.now));
    auto scans = h.radio.scan_calls, captures = h.radio.capture_calls;
    assert(!h.supervisor.transition(Mode::Escudo, h.context.now));
    snapshot(h, 1, "stopped");
    assert(h.radio.scan_calls == scans && h.radio.capture_calls == captures &&
           storage.writes == writes);
    assert(!h.command("escudo.capture_start").error);
    feed(h, fixtures::deauth());
    assert(!detections(h)); // Four observations from the prior mode are gone.
    // Even an unavailable NVS read retains the accepted target in this boot.
    assert(!h.supervisor.transition(Mode::Idle, h.context.now));
    storage.fail = Fail::OpenRead;
    assert(!h.supervisor.transition(Mode::Escudo, h.context.now));
    h.tick();
    snapshot(h, 1, "stopped");
    assert(detections(h, "resource_error") == 1);
    storage.fail = Fail::None;
  }
  Harness reboot;
  snapshot(reboot, 1, "stopped");
  assert(!reboot.radio.scan_calls && !reboot.radio.channel_calls && !reboot.radio.capture_calls);
  auto state = reboot.state();
  assert(state.find("seen_uptime_ms") == std::string::npos &&
         state.find("900000") == std::string::npos);
  auto page = reboot.command("escudo.networks", "{\"after\":null,\"limit\":4}");
  assert(page.error && !strcmp(page.error, "invalid_state"));
  reboot.tick();
  assert(!detections(reboot) && reboot.context.events.empty());
  assert(!reboot.command("escudo.capture_start").error);
  // A fresh insecure peer alone cannot use persisted security/RSSI/uptime as
  // a current observation of the target, even early in the new boot.
  feed(reboot, fixtures::beacon(false, 2, 11), 11);
  assert(!detections(reboot));
  feed(reboot, fixtures::deauth());
  assert(!detections(reboot)); // No deauth window resurrected from the old boot.
  for (unsigned i = 0; i < 4; ++i) feed(reboot, fixtures::deauth());
  assert(detections(reboot, "suspicious_disconnect") == 1);
  const auto &disconnect = reboot.context.events.back();
  assert(disconnect.find("wpa2_personal") != std::string::npos &&
         disconnect.find("ccmp") != std::string::npos);
  // Both sides now have real observations in this boot: the existing detector
  // must still report the downgrade (including the peer observed first).
  feed(reboot, fixtures::beacon(true));
  assert(detections(reboot, "evil_twin") == 1);
  for (unsigned i = 0; i < 5; ++i) feed(reboot, fixtures::deauth());
  assert(detections(reboot, "deauth_burst") == 1);
  assert(!reboot.command("escudo.capture_stop").error);
}
int main() {
  helper_tests();
  invalid_storage_tests();
  selection_failure_tests();
  restoration_tests();
  std::cout << "Escudo target persistence: PASS (real helper/module; NVS/radio SDK doubles, no physical durability claim)\n";
}
