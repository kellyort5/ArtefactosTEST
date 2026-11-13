#include "frame_fixtures.hpp"
#include "test_support.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
namespace fs = std::filesystem;
using namespace fixtures;
// Host adapter only. Separate invocations reopen real files; production keeps
// the existing NvsStore/nvs_commit. This proves neither NVS nor power-loss RF.
struct FileStore : Store {
  fs::path directory;
  bool absent = false, fail_write = false;
  explicit FileStore(fs::path path) : directory(std::move(path)) {
    fs::create_directories(directory);
  }
  bool read(size_t slot, char *out, size_t *n) override {
    auto path = directory / ("slot-" + std::to_string(slot));
    absent = !fs::exists(path);
    if (absent)
      return false;
    std::ifstream f(path, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(f)), {});
    if (!f || bytes.size() > *n)
      return false;
    memcpy(out, bytes.data(), bytes.size());
    *n = bytes.size();
    return true;
  }
  bool write(size_t slot, const char *data, size_t n) override {
    if (fail_write)
      return false;
    std::ofstream f(directory / ("slot-" + std::to_string(slot)),
                    std::ios::binary);
    f.write(data, n);
    f.flush();
    return bool(f);
  }
  bool missing() const override { return absent; }
};
struct RecoveryContext : ModuleContext {
  ModuleLease lease;
  Journal &journal;
  ModuleOutputQueue output;
  const char *boot;
  uint64_t now = 1001, seq = 0;
  bool connected = false;
  std::vector<std::string> committed, live, errors;
  RecoveryContext(ModuleSupervisor &s, Radio &r, Journal &j, const char *b)
      : lease(s, Mode::Escudo, r), journal(j), boot(b) {}
  uint64_t now_ms() const override { return now; }
  ModuleResources &resources() override { return lease; }
  bool emit_event(const char *name, const char *severity, const char *details,
                  bool simulated) override {
    assert(!simulated); // input boundary is host radio, never claim real RF
    ModuleOutput out;
    out.mode = Mode::Escudo;
    out.simulated = true;
    assert(strlen(name) < sizeof(out.name) &&
           strlen(details) < sizeof(out.json));
    strcpy(out.name, name);
    strcpy(out.severity, severity);
    strcpy(out.json, details);
    return output.push(out);
  }
  bool publish_metrics(uint32_t interval, const char *json) override {
    ModuleOutput out;
    out.metric = true;
    out.mode = Mode::Escudo;
    out.interval_ms = interval;
    strcpy(out.json, json);
    return output.push(out);
  }
  void drain() {
    ModuleOutput out;
    while (output.pop(out)) {
      if (out.metric)
        continue; // production metrics are ephemeral, not journal
      Envelope env = {"node-recovery", boot, ++seq, now};
      env.version = 2;
      char json[MaxMessage + 1];
      if (auto error = persist_module_event(journal, out, env, json)) {
        errors.emplace_back(error);
        assert(!*json);
        continue;
      }
      committed.emplace_back(json);
      if (connected)
        live.emplace_back(json);
    }
  }
};
struct RecoveryRig {
  Radio radio;
  Runtime runtime;
  ModuleSupervisor supervisor{runtime, radio};
  RecoveryContext context;
  Escudo module;
  RecoveryRig(Journal &j, const char *boot = "boot-offline")
      : context(supervisor, radio, j, boot) {
    assert(supervisor.add(Mode::Escudo, module, context));
    assert(!supervisor.transition(Mode::Escudo, context.now));
  }
  void tick() {
    supervisor.tick(context.now);
    context.drain();
  }
  void command(const char *op, const char *args = "{}") {
    auto json = cJSON_Parse(args);
    assert(json);
    ModuleCommand request{op, json, "recovery-operation"};
    ModuleReply reply;
    module.handle(request, reply);
    cJSON_Delete(json);
    assert(!reply.error);
  }
  void start() {
    command("escudo.scan");
    radio.records = {network(1), network(2, "LAB", 11)};
    radio.done();
    tick();
    command("escudo.select_target",
            "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}");
    tick();
    command("escudo.capture_start");
    context.drain();
  }
  void feed(const Bytes &bytes, uint8_t channel = 6) {
    context.now += 10;
    radio.feed(frame(bytes, 1, channel, context.now));
    tick();
  }
};
static uint64_t cursor(const std::string &wire) {
  auto j = cJSON_Parse(wire.c_str());
  assert(j);
  auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
  auto n = number(p, "journal_seq");
  cJSON_Delete(j);
  return n;
}
static std::string read_event(Journal &j, uint64_t c) {
  char bytes[MaxMessage + 1];
  size_t n = sizeof(bytes);
  assert(j.read(c, bytes, &n));
  return std::string(bytes, n);
}
static Envelope env(const char *boot, uint64_t seq) {
  Envelope e = {"node-recovery", boot, seq, 2000};
  e.version = 2;
  return e;
}
// Exercises the production parser, journal page/read and ACK serializer. The
// transport is a host sink; no Android/BLE delivery claim is made here.
static Result replay(Journal &journal, uint64_t after, size_t limit,
                     unsigned id, bool wire = false,
                     const char *boot = "boot-reconnected") {
  char command[MaxMessage + 1], ack[MaxMessage + 1], name[40];
  snprintf(name, sizeof(name), "read-page-%u", id);
  auto p = cJSON_CreateObject();
  cJSON_AddStringToObject(p, "command_id", name);
  cJSON_AddStringToObject(p, "target_boot_id", boot);
  cJSON_AddNumberToObject(p, "expected_revision", 0);
  cJSON_AddNumberToObject(p, "expires_at_uptime_ms", 32000);
  cJSON_AddStringToObject(p, "operation", "read_events");
  auto args = cJSON_AddObjectToObject(p, "args");
  cJSON_AddNumberToObject(args, "after_cursor", after);
  cJSON_AddNumberToObject(args, "limit", limit);
  assert(serialize(envelope(env(boot, 100 + id), "command", p), command));
  Command parsed;
  assert(!parsed.parse_v2(command, strlen(command)));
  assert(!v2::admission(parsed, Channel::Ble, Mode::Escudo, true, false));
  uint64_t cursors[8];
  size_t n = 0;
  auto result = journal.page(number(parsed.args, "after_cursor"),
                             number(parsed.args, "limit"), cursors, &n);
  strcpy(result.id, name);
  if (wire)
    std::cout << command << '\n';
  for (size_t i = 0; i < n; ++i) {
    auto original = read_event(journal, cursors[i]);
    assert(cursor(original) == cursors[i]);
    if (wire)
      std::cout << original << '\n';
  }
  assert(serialize(envelope(env(boot, 200 + id), "ack",
                            module_ack_payload(result, "applied")),
                   ack));
  auto a = v2::parse(ack, strlen(ack));
  assert(a && v2::validate("ack", a));
  cJSON_Delete(a);
  if (wire)
    std::cout << ack << '\n';
  return result;
}
static void write_offline(const fs::path &root) {
  FileStore store(root / "persistent");
  Journal journal(store, 8);
  journal.recover();
  RecoveryRig rig(journal);
  rig.start();
  rig.feed(beacon(true));
  for (unsigned i = 0; i < 10; ++i)
    rig.feed(deauth());
  assert(rig.context.errors.empty() && rig.context.live.empty());
  std::string incident;
  for (auto &wire : rig.context.committed) {
    auto j = cJSON_Parse(wire.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    auto d = cJSON_GetObjectItemCaseSensitive(p, "details");
    if (!strcmp(text(p, "name"), "escudo.detection") &&
        !strcmp(text(d, "detector"), "deauth_burst"))
      incident = wire;
    cJSON_Delete(j);
  }
  assert(!incident.empty());
  std::ofstream expected(root / "expected.json");
  expected << incident;
  expected.flush();
  assert(expected);
  assert(read_event(journal, cursor(incident)) == incident);
}
static void recover_offline(const fs::path &root, bool wire = false) {
  std::ifstream expected(root / "expected.json");
  std::string original((std::istreambuf_iterator<char>(expected)), {});
  assert(!original.empty());
  FileStore reopened(root / "persistent");
  Journal journal(reopened, 8);
  journal.recover();
  assert(!journal.degraded &&
         read_event(journal, cursor(original)) == original);
  auto previous = journal.highest;
  ModuleOutput boot;
  boot.simulated = true;
  strcpy(boot.name, "system.booted");
  strcpy(boot.severity, "info");
  strcpy(boot.json, "{}");
  char json[MaxMessage + 1];
  assert(
      !persist_module_event(journal, boot, env("boot-reconnected", 1), json));
  assert(journal.highest == previous + 1 && cursor(json) == previous + 1);
  assert(read_event(journal, cursor(original)) ==
         original); // old identity/time intact
  auto first = replay(journal, 0, 8, 1, wire);
  auto duplicate = replay(journal, 0, 8, 2, wire);
  assert(first.next == duplicate.next && first.count == duplicate.count &&
         !first.gap);
  for (size_t i = 0; i < first.count; ++i)
    assert(first.cursors[i] == duplicate.cursors[i]);
  auto empty = replay(journal, first.next, 8, 3, wire);
  assert(!empty.more && !empty.count && empty.next == first.next);
}
static void gap_and_coverage(const fs::path &root, bool wire = false) {
  FileStore store(root / "small-ring");
  Journal journal(store, 3);
  journal.recover();
  RecoveryRig rig(journal, "boot-gap");
  rig.start();
  auto oldest = rig.context.committed.front();
  rig.context.now += 5000;
  rig.tick();
  rig.context.now += 5000;
  rig.tick();
  assert(journal.count() == 3 && journal.first() > cursor(oldest));
  auto first = replay(journal, 0, 2, 4, wire, "boot-gap");
  assert(first.gap && first.more && first.from == journal.first() &&
         first.to == journal.highest);
  auto last = replay(journal, first.next, 2, 5, wire, "boot-gap");
  assert(!last.more && last.next == journal.highest);
  bool coverage = false;
  for (uint64_t c = journal.first(); c <= journal.highest; ++c) {
    auto text = read_event(journal, c);
    auto j = cJSON_Parse(text.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    if (!strcmp(sentinel::str(p, "name"), "escudo.coverage")) {
      auto d = cJSON_GetObjectItemCaseSensitive(p, "details");
      assert(cJSON_GetObjectItemCaseSensitive(d, "duration_ms"));
      coverage = true;
    }
    cJSON_Delete(j);
  }
  assert(coverage);
  // Replay is read-only: it does not stop capture or drop its selection.
  ModuleSnapshot snapshot;
  rig.module.snapshot(snapshot);
  assert(snapshot.target_selected && snapshot.capabilities.detectors &&
         strstr(snapshot.state, "\"capture_status\":\"running\""));
  assert(rig.radio.physical_capture); // host radio boundary, not a device claim
  rig.command("escudo.capture_stop");
  rig.context.drain();
  rig.command("escudo.capture_start");
  rig.context.drain();
  rig.command(
      "escudo.select_target",
      "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:02\",\"channel\":11}");
  rig.tick();
  for (unsigned i = 0; i < 10; ++i) {
    auto b = deauth();
    b[15] = b[21] = 2;
    rig.feed(b, 11);
  }
  bool detection = false;
  for (auto &event : rig.context.committed)
    if (event.find("deauth_burst") != std::string::npos &&
        event.find("02:00:00:00:00:02") != std::string::npos)
      detection = true;
  assert(detection);
}
static void failure_test(const fs::path &root) {
  FileStore store(root / "failed-store");
  Journal journal(store, 3);
  RecoveryRig rig(journal);
  rig.context.connected = true;
  rig.start();
  auto high = journal.highest;
  auto live = rig.context.live.size();
  store.fail_write = true;
  rig.context.now += 5000;
  rig.tick();
  assert(journal.degraded && journal.highest == high &&
         rig.context.live.size() == live);
  assert(!rig.context.errors.empty() &&
         rig.context.errors.back() == "resource_error");
  rig.command(
      "escudo.capture_stop"); // control remains functional on storage failure
  assert(!rig.radio.physical_capture);
}
int main(int argc, char **argv) {
  assert(argc == 3);
  fs::path root(argv[2]);
  fs::create_directories(root);
  if (!strcmp(argv[1], "write"))
    write_offline(root);
  else if (!strcmp(argv[1], "recover"))
    recover_offline(root);
  else if (!strcmp(argv[1], "checks")) {
    gap_and_coverage(root);
    failure_test(root);
  } else if (!strcmp(argv[1], "wire")) {
    write_offline(root);
    recover_offline(root, true);
    return 0;
  } else if (!strcmp(argv[1], "wire-gap")) {
    gap_and_coverage(root, true);
    return 0;
  } else
    assert(false);
  std::cout << "Escudo recovery " << argv[1]
            << ": PASS (host; no NVS/BLE claims)\n";
}
