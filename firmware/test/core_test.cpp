#include "core.h"
#include "protocol_v2.h"
#include "wire.h"
extern void run_module_tests();
extern void module_wire_fixtures();
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace sentinel;
static bool reject_digest_buffer = false;
static size_t live_digest_allocations = 0;
static void *digest_buffer = nullptr;
static void *digest_test_malloc(size_t n) {
  if (reject_digest_buffer && n == MaxMessage + 1)
    return nullptr;
  void *p = std::malloc(n);
  if (p) {
    ++live_digest_allocations;
    if (n == MaxMessage + 1) {
      assert(!digest_buffer);
      digest_buffer = p;
    }
  }
  return p;
}
static void digest_test_free(void *p) {
  if (p) {
    if (p == digest_buffer) {
      const auto *bytes = static_cast<const unsigned char *>(p);
      for (size_t n = 0; n < MaxMessage + 1; ++n) assert(bytes[n] == 0);
      digest_buffer = nullptr;
    }
    assert(live_digest_allocations > 0);
    --live_digest_allocations;
    std::free(p);
  }
}
static std::string fixture(const char *name) {
  std::ifstream f(std::string("contracts/fixtures/") + name + ".json");
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}
static std::string print(cJSON *j) {
  char *p = cJSON_PrintUnformatted(j);
  std::string s = p;
  cJSON_free(p);
  cJSON_Delete(j);
  return s;
}
struct MemoryStore : Store {
  std::string data[64];
  bool fail = false;
  bool absent = true;
  bool read(size_t i, char *out, size_t *n) override {
    absent = data[i].empty();
    if (absent || fail || data[i].size() > *n)
      return false;
    *n = data[i].size();
    memcpy(out, data[i].data(), *n);
    return true;
  }
  bool write(size_t i, const char *p, size_t n) override {
    if (fail)
      return false;
    data[i].assign(p, n);
    return true;
  }
  bool missing() const override { return absent; }
};
struct FakeResources : Resources {
  bool stop_ok = true, start_ok = true, leased = false;
  int starts = 0, stops = 0;
  bool stop_network() override {
    ++stops;
    if (stop_ok)
      leased = false;
    return stop_ok;
  }
  bool network_released() const override { return !leased; }
  bool start_cebo() override {
    assert(!leased);
    ++starts;
    leased = start_ok;
    return start_ok;
  }
};
static std::string ev(uint64_t seq) {
  char out[2049];
  assert(serialize(
      envelope({"node-test", "boot-test", seq, 10}, "event",
               event_payload(seq, "system.booted", Profile::Idle, nullptr)),
      out));
  return out;
}
int main(int argc, char **argv) {
  if (argc == 1) {
    run_module_tests();
    MemoryStore store;
    Journal j(store);
    Envelope e = {"node-test", "boot-test", 1, 10};
    e.version = 2;
    char json[MaxMessage + 1];
    assert(j.append(ev(1).c_str(), ev(1).size(), 1));
    assert(
        serialize(envelope(e, "event",
                           module_event_payload(2, "system.booted", Mode::Idle,
                                                nullptr, "{}", "info", true)),
                  json));
    assert(j.append(json, strlen(json), 2));
    Journal recovered(store);
    recovered.recover();
    assert(recovered.highest == 2 && recovered.count() == 2);
    size_t n = sizeof(json);
    assert(recovered.read(2, json, &n) && strstr(json, "simulated"));
  }
  if (argc == 2 && std::string(argv[1]) == "wire2") {
    module_wire_fixtures();
    return 0;
  }
  if (argc == 3 && std::string(argv[1]) == "v2") {
    std::ifstream f(argv[2], std::ios::binary);
    std::stringstream s;
    s << f.rdbuf();
    auto data = s.str();
    auto j = v2::parse(data.c_str(), data.size());
    const char *type = str(j, "type");
    bool ok = j && v2::validate(type ? type : "V2Metadata", j);
    cJSON_Delete(j);
    return ok ? 0 : 2;
  }
  if (argc == 3 && std::string(argv[1]) == "parse") {
    std::ifstream f(argv[2], std::ios::binary);
    std::stringstream stream;
    stream << f.rdbuf();
    std::string data = stream.str();
    Command c;
    auto error = c.parse(data.data(), data.size());
    std::cout << (error ? error : "valid") << '\n';
    return error ? 2 : 0;
  }
  if (argc == 2 && std::string(argv[1]) == "wire") {
    char out[2049];
    Runtime r;
    Result ack;
    strcpy(ack.id, "cmd-test");
    Envelope e{"node-test", "boot-test", 1, 10};
    assert(serialize(
        envelope(e, "state",
                 state_payload(r, "advertising", "disconnected", "disconnected",
                               false, true, true, true)),
        out));
    std::cout << out << '\n';
    assert(serialize(envelope(e, "ack", ack_payload(ack, "accepted")), out));
    std::cout << out << '\n';
    ack.page = true;
    ack.from = 1;
    ack.to = 8;
    ack.next = 8;
    assert(serialize(envelope(e, "ack", ack_payload(ack, "applied")), out));
    std::cout << out << '\n';
    std::cout << ev(1) << '\n';
    auto p = cJSON_CreateObject();
    cJSON_AddNumberToObject(p, "free_heap_bytes", 123);
    cJSON_AddNumberToObject(p, "min_free_heap_bytes", 100);
    cJSON_AddNumberToObject(p, "journal_count", 1);
    assert(serialize(envelope(e, "metrics", p), out));
    std::cout << out << '\n';
    return 0;
  }
  for (auto name :
       {"get_state.valid", "set_mode.valid", "configure_connectivity.valid",
        "set_time.valid", "read_events.valid", "start_capture.valid",
        "start_honeypot.valid"}) {
    auto s = fixture(name);
    assert(!s.empty());
    Command c;
    auto error = c.parse(s.c_str(), s.size());
    if (error)
      std::cerr << name << ':' << error << '\n';
    assert(!error);
  }
  {
    SlotPool pool;
    for (size_t i = 0; i < QueueSize; ++i)
      assert(pool.acquire() == static_cast<int>(i));
    assert(pool.acquire() == -1);
    pool.release(3);
    assert(pool.acquire() == 3);
    assert(pool.acquire() == -1);
  }
  {
    Profile p;
    assert(!strcmp(requested_profile("sanitizador", &p), "not_implemented"));
    assert(!requested_profile("escudo", &p) && p == Profile::Escudo);
  }
  for (auto op : {"start_capture.valid", "start_honeypot.valid"}) {
    auto input = fixture(op);
    Command command;
    assert(!command.parse(input.c_str(), input.size()));
    assert(!strcmp(operation_error(command), "not_implemented"));
  }
  {
    PendingCommands pending;
    uint8_t digest[32] = {1};
    assert(pending.admit("cmd", digest, Channel::Ble, 1, 1) == 0);
    assert(pending.admit("cmd", digest, Channel::Mqtt, 0, 1) == -2);
    digest[0] = 2;
    assert(pending.admit("cmd", digest, Channel::Mqtt, 0, 1) == -3);
    pending.release(0);
    assert(pending.admit("cmd", digest, Channel::Mqtt, 0, 1) == 0);
  }
  auto base = fixture("get_state.valid");
  {
    Command a;
    assert(!a.parse(base.c_str(), base.size()));
    auto reordered = cJSON_Parse(base.c_str());
    auto payload = cJSON_GetObjectItemCaseSensitive(reordered, "payload");
    auto field = cJSON_DetachItemFromObjectCaseSensitive(payload, "command_id");
    cJSON_AddItemToObject(payload, "command_id", field);
    auto data = print(reordered);
    Command b;
    assert(!b.parse(data.c_str(), data.size()));
    uint8_t left[32], right[32];
    assert(digest_command(a, left) && digest_command(b, right) &&
           !memcmp(left, right, 32));
    char *before = cJSON_PrintUnformatted(a.root);
    const std::string original(before);
    cJSON_free(before);
    cJSON_Hooks hooks{digest_test_malloc, digest_test_free};
    cJSON_InitHooks(&hooks);
    reject_digest_buffer = true;
    uint8_t rejected[32];
    memset(rejected, 0xa5, sizeof(rejected));
    assert(!digest_command(a, rejected));
    assert(live_digest_allocations == 0);
    for (auto byte : rejected) assert(byte == 0xa5);
    reject_digest_buffer = false;
    assert(digest_command(a, right) && !memcmp(left, right, sizeof(left)));
    assert(live_digest_allocations == 0);
    cJSON_InitHooks(nullptr);
    char *after = cJSON_PrintUnformatted(a.root);
    assert(original == after);
    cJSON_free(after);
  }
  {
    Command c;
    assert(!c.parse(base.c_str(), base.size()));
    uint8_t hash[32] = {1};
    CommandGate gate;
    const Result *cached = nullptr;
    auto dev = str(c.root, "device_id"), boot = str(c.root, "boot_id");
    uint64_t now = c.expiry - 1000;
    assert(!gate.check(c, dev, boot, c.revision, now, true, false, hash,
                       &cached) &&
           !cached);
    assert(!strcmp(gate.check(c, dev, "boot-other", c.revision, now, true,
                              false, hash, &cached),
                   "wrong_boot"));
    assert(!strcmp(gate.check(c, dev, boot, c.revision, c.expiry, true, false,
                              hash, &cached),
                   "expired"));
    assert(!strcmp(gate.check(c, dev, boot, c.revision, c.expiry - 30001, true,
                              false, hash, &cached),
                   "expired"));
    assert(!strcmp(gate.check(c, dev, boot, c.revision + 1, now, true, false,
                              hash, &cached),
                   "revision_conflict"));
    assert(!strcmp(
        gate.check(c, dev, boot, c.revision, now, true, true, hash, &cached),
        "unauthorized"));
    Result result;
    strcpy(result.id, c.id);
    result.revision = c.revision + 1;
    gate.remember(c, hash, result);
    assert(!gate.check(c, dev, boot, c.revision + 1, now, true, false, hash,
                       &cached) &&
           cached && cached->revision == c.revision + 1);
    hash[0] = 2;
    assert(!strcmp(
        gate.check(c, dev, boot, c.revision, now, true, false, hash, &cached),
        "id_conflict"));
  }
  {
    auto root = cJSON_Parse(base.c_str());
    cJSON_ReplaceItemInObject(root, "version", cJSON_CreateNumber(2));
    auto s = print(root);
    Command c;
    assert(!strcmp(c.parse(s.c_str(), s.size()), "unsupported_version"));
  }
  for (auto suffix : {std::string("\"device_id\":\"x\\u0000y\""),
                      std::string("\"device_id\\u0000\":\"x\""),
                      std::string("\"device_id\":\"x") + char(0xff) + "\""}) {
    auto s = base;
    auto a = s.find("\"device_id\"");
    auto b = s.find(',', a);
    s.replace(a, b - a, suffix);
    Command c;
    assert(c.parse(s.c_str(), s.size()));
  }
  {
    auto s = base;
    auto a = s.find("\"device_id\"");
    auto b = s.find(',', a);
    s.replace(a, b - a, "\"device_id\":\"x\",\"device_id\":\"y\"");
    Command c;
    assert(c.parse(s.c_str(), s.size()));
  }
  assert(utc_valid("2024-02-29T12:00:00.123456Z"));
  assert(!utc_valid("2023-02-29T12:00:00Z"));
  assert(!utc_valid("2024-01-01T24:00:00Z"));
  assert(mqtt_uri_valid("mqtts://[::1]:8883"));
  assert(!mqtt_uri_valid("mqtts://host:65536"));
  assert(!mqtt_uri_valid("mqtts://user:password@host"));
  {
    Frame f;
    std::string body(2048, 'x');
    uint64_t now = 0;
    for (size_t offset = 0; offset < body.size();) {
      size_t n = std::min<size_t>(12, body.size() - offset);
      std::vector<uint8_t> p(8 + n);
      p[0] = 7;
      p[4] = offset;
      p[5] = offset >> 8;
      p[6] = 0;
      p[7] = 8;
      memcpy(p.data() + 8, body.data() + offset, n);
      int rc = f.push(p.data(), p.size(), now++);
      offset += n;
      assert(rc == (offset == body.size() ? 1 : 0));
    }
    assert(!memcmp(f.bytes, body.data(), 2048));
    f.reset();
    uint8_t part[] = {1, 0, 0, 0, 0, 0, 2, 0, 'a'};
    assert(f.push(part, sizeof(part), 100) == 0);
    part[4] = 1;
    assert(f.push(part, sizeof(part), 10100) == -1);
    assert(!f.used);
    part[4] = 0;
    assert(f.push(part, sizeof(part), 0) == 0);
    f.reset();
    part[4] = 1;
    assert(f.push(part, sizeof(part), 1) == -1);
  }
  {
    Button b;
    assert(b.update(true, 0) == 0);
    assert(b.update(true, 50) == 0);
    assert(b.update(true, 3050) == 2);
    assert(b.update(false, 3060) == 0);
    assert(b.update(false, 3110) == 0);
    assert(b.update(true, 3200) == 0);
    assert(b.update(true, 3250) == 0);
    assert(b.update(false, 3300) == 0);
    assert(b.update(false, 3350) == 1);
  }
  {
    Runtime r;
    FakeResources resources;
    assert(!r.transition(Profile::Cebo, resources));
    assert(resources.leased);
    assert(!r.transition(Profile::Escudo, resources));
    assert(!resources.leased && r.revision == 2);
    resources.stop_ok = false;
    int starts = resources.starts;
    assert(r.transition(Profile::Cebo, resources));
    assert(resources.starts == starts && !strcmp(r.phase, "error"));
    resources.stop_ok = true;
    assert(!r.transition(Profile::Idle, resources));
    resources.start_ok = false;
    assert(r.transition(Profile::Cebo, resources));
    assert(!resources.leased);
  }
  {
    MemoryStore store;
    Journal journal(store, 3);
    journal.recover();
    for (uint64_t i = 1; i <= 5; ++i) {
      auto s = ev(i);
      assert(journal.append(s.c_str(), s.size(), i));
    }
    assert(journal.count() == 3 && journal.first() == 3);
    Journal rebooted(store, 3);
    rebooted.recover();
    assert(rebooted.highest == 5 && rebooted.first() == 3);
    uint64_t cursors[8];
    size_t n;
    auto r = rebooted.page(0, 2, cursors, &n);
    assert(n == 2 && cursors[0] == 3 && cursors[1] == 4 && r.gap && r.more &&
           r.next == 4);
    auto repeat = rebooted.page(0, 2, cursors, &n);
    assert(repeat.next == r.next && repeat.cursors[0] == 3);
    char out[2049];
    size_t len = sizeof(out);
    assert(rebooted.read(3, out, &len) && std::string(out, len) == ev(3));
    store.fail = true;
    auto s = ev(6);
    assert(!rebooted.append(s.c_str(), s.size(), 6) && rebooted.highest == 5 &&
           rebooted.degraded);
    store.fail = false;
    store.data[1] = "broken";
    Journal corrupt(store, 3);
    corrupt.recover();
    assert(corrupt.degraded && corrupt.highest == 4);
  }
  std::cout << "commands, framing, button, supervisor, ring "
               "recovery/overwrite/failure: PASS\n";
}
