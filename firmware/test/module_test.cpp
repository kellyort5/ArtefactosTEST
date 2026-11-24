#include "module_runtime.h"
#include "protocol_v2.h"
#include "wire.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace sentinel;
extern Module &test_consumer();
static std::string fixture(const char *name) {
  std::ifstream f(std::string("contracts/fixtures/v2/") + name + ".valid.json");
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}
struct Radio final : Resources, ModuleResources {
  bool leased = false, stop_ok = true, nat = false;
  unsigned starts = 0, channels = 0;
  Mode mode = Mode::Idle;
  bool stop_network() override {
    if (stop_ok) {
      leased = nat = false;
      mode = Mode::Idle;
    }
    return stop_ok;
  }
  bool network_released() const override { return !leased; }
  bool start_cebo() override { return start_profile(Mode::Cebo); }
  bool start_profile(Mode p) override {
    assert(!leased);
    mode = p;
    leased = p != Mode::Idle;
    ++starts;
    return true;
  }
  bool set_channel(uint8_t) override {
    ++channels;
    return true;
  }
  bool scan_passive() override { return true; }
  size_t scan_results(ScanNetwork *, size_t) override { return 0; }
  bool capture(bool) override { return true; }
  bool configure_upstream(const UpstreamConfig &) override { return true; }
  bool configure_ap(const ApConfig &) override { return true; }
  bool napt(bool b) override {
    nat = b;
    return true;
  }
  esp_netif_obj *sta_netif() const override { return nullptr; }
  esp_netif_obj *ap_netif() const override { return nullptr; }
  bool poll(RadioEvent &) override { return false; }
  uint64_t dropped() const override { return 0; }
};
struct Context final : ModuleContext {
  Radio &r;
  explicit Context(Radio &radio) : r(radio) {}
  uint64_t now_ms() const override { return 1000; }
  ModuleResources &resources() override { return r; }
  bool emit_event(const char *, const char *, const char *, bool) override {
    return true;
  }
  bool publish_metrics(uint32_t, const char *) override { return true; }
};
struct FixtureModule final : Module {
  Mode mode = Mode::Escudo;
  bool running = false, release = true, stop_requested = false, job = false;
  unsigned starts = 0, handled = 0;
  bool init(ModuleContext &) override { return true; }
  bool start() override {
    ++starts;
    running = true;
    stop_requested = false;
    return true;
  }
  void request_stop() override {
    stop_requested = true;
    if (release)
      running = false;
  }
  bool stopped() const override { return !running; }
  void tick(uint64_t) override {
    if (stop_requested && release)
      running = false;
  }
  void handle(const ModuleCommand &, ModuleReply &r) override {
    ++handled;
    r.job = job;
  }
  void snapshot(ModuleSnapshot &s) const override {
    auto text =
        fixture(mode == Mode::Escudo ? "states/escudo" : "states/sanitizador");
    auto j = cJSON_Parse(text.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    auto m = cJSON_GetObjectItemCaseSensitive(p, "module");
    char *raw = cJSON_PrintUnformatted(m);
    strcpy(s.state, raw);
    cJSON_free(raw);
    cJSON_Delete(j);
    s.capabilities.capture = mode == Mode::Escudo;
    s.capabilities.sanitizador = mode == Mode::Sanitizador;
    s.capabilities.detectors = true;
    s.target_selected = true;
  }
};
void run_module_tests() {
  Radio radio;
  Context context(radio);
  Runtime runtime;
  ModuleSupervisor modules(runtime, radio);
  assert(modules.available(Mode::Idle) && !modules.available(Mode::Escudo));
  assert(!strcmp(modules.transition(Mode::Sanitizador, 0), "not_implemented"));
  assert(modules.add(Mode::Cebo, test_consumer(), context));
  assert(!modules.add(Mode::Cebo, test_consumer(), context));
  FixtureModule escudo;
  assert(modules.add(Mode::Escudo, escudo, context));
  ModuleLease elease(modules, Mode::Escudo, radio),
      clease(modules, Mode::Cebo, radio);
  assert(!elease.set_channel(1) && !clease.set_channel(1));
  assert(!modules.transition(Mode::Escudo, 100));
  assert(runtime.profile == Mode::Escudo && escudo.starts == 1);
  assert(elease.set_channel(6) && !clease.set_channel(6) &&
         !elease.napt(true) && !radio.nat);
  Command capture;
  auto text = fixture("commands/escudo.capture_start");
  assert(!capture.parse_v2(text.c_str(), text.size()));
  assert(!strcmp(
      v2::admission(capture, Channel::Ble, runtime.profile, false, false),
      "unauthorized"));
  assert(
      !strcmp(v2::admission(capture, Channel::Ble, runtime.profile, true, true),
              "invalid_message"));
  assert(!strcmp(v2::admission(capture, Channel::Mqtt, Mode::Cebo, true, false),
                 "unauthorized"));
  assert(!strcmp(v2::admission(capture, Channel::Mqtt, Mode::Idle, true, false),
                 "invalid_state"));
  assert(!strcmp(v2::new_context(capture, Mode::Cebo, true), "invalid_state"));
  assert(
      !strcmp(v2::new_context(capture, Mode::Escudo, false), "invalid_state"));
  ModuleReply reply;
  assert(!modules.dispatch(capture, reply) && escudo.handled == 1);
  Command scan;
  text = fixture("commands/escudo.scan");
  assert(!scan.parse_v2(text.c_str(), text.size()));
  assert(!strcmp(modules.dispatch(scan, reply),
                 "invalid_message")); // scan must return a job
  escudo.job = true;
  reply = {};
  assert(!modules.dispatch(scan, reply) && reply.job);
  // Preserve admission/cache/new-context order: retries succeed with old
  // revision.
  CommandGate gate;
  uint8_t digest[32];
  assert(digest_command(capture, digest));
  const Result *cached = nullptr;
  assert(!gate.check(capture, "node-example", "boot-example", 3, 1000, true,
                     false, digest, &cached));
  Result completed;
  strcpy(completed.id, capture.id);
  gate.remember(capture, digest, completed);
  assert(!gate.check(capture, "node-example", "boot-example", 4, 1001, true,
                     false, digest, &cached) &&
         cached);
  assert(!strcmp(gate.check(capture, "node-example", "boot-example", 4, 1001,
                            false, false, digest, &cached),
                 "unauthorized"));
  assert(!strcmp(gate.check(capture, "node-example", "boot-example", 4, 31000,
                            true, false, digest, &cached),
                 "expired"));
  assert(!strcmp(gate.check(capture, "node-example", "boot-example", 4, 1001,
                            true, false, digest, &cached, "unauthorized"),
                 "unauthorized"));
  assert(!strcmp(gate.check(capture, "node-example", "boot-example", 4, 31000,
                            true, false, digest, &cached, "unauthorized"),
                 "expired"));
  ++digest[0];
  assert(!strcmp(gate.check(capture, "node-example", "boot-example", 4, 1001,
                            true, false, digest, &cached),
                 "id_conflict"));
  escudo.release = false;
  unsigned starts = radio.starts;
  assert(!modules.transition(Mode::Cebo, 1000) && modules.transitioning());
  assert(!strcmp(modules.transition(Mode::Idle, 1001), "busy"));
  modules.tick(5999);
  assert(modules.transitioning() && runtime.profile == Mode::Escudo &&
         radio.starts == starts);
  modules.tick(6000);
  assert(!modules.transitioning() && !strcmp(runtime.phase, "error") &&
         !strcmp(runtime.error, "timeout"));
  assert(runtime.profile == Mode::Escudo && radio.starts == starts &&
         !clease.set_channel(1));
  assert(!strcmp(modules.transition(Mode::Cebo, 7000), "invalid_state"));
  char failure[MaxMessage + 1];
  Envelope failed_env = {"node-example", "boot-example", 1, 6000};
  failed_env.version = 2;
  assert(serialize(envelope(failed_env, "state",
                            module_state_payload(runtime, modules, "connected",
                                                 "disconnected", "disconnected",
                                                 true, true, true)),
                   failure));
  assert(strstr(failure, "timeout") && strstr(failure, "error"));
  escudo.release = true;
  assert(!modules.transition(Mode::Idle, 7001));
  assert(runtime.profile == Mode::Idle && !radio.leased);
  assert(!modules.transition(Mode::Cebo, 8000));
  Command start;
  text = fixture("commands/cebo.start");
  assert(!start.parse_v2(text.c_str(), text.size()));
  reply = {};
  assert(!strcmp(modules.dispatch(start, reply),
                 "not_implemented")); // registered != implemented HTTP/Telnet
  radio.stop_ok = false;
  assert(!strcmp(modules.transition(Mode::Escudo, 9000), "resource_error"));
  assert(escudo.starts == 1 && !strcmp(runtime.phase, "error"));
  radio.stop_ok = true;
  assert(!modules.transition(Mode::Idle, 9001));
  assert(modules.next() == Mode::Escudo);
  assert(!modules.transition(Mode::Escudo, 10000));
  assert(modules.next() == Mode::Cebo);
  assert(!modules.transition(Mode::Idle, 10001));
  FixtureModule sanitizer;
  sanitizer.mode = Mode::Sanitizador;
  assert(modules.add(Mode::Sanitizador, sanitizer, context));
  assert(!modules.transition(Mode::Escudo, 10002));
  assert(modules.next() == Mode::Sanitizador);
  assert(!modules.transition(Mode::Sanitizador, 10003));
  ModuleLease slease(modules, Mode::Sanitizador, radio);
  assert(!elease.napt(true) && slease.napt(true) && radio.nat);
  assert(modules.next() == Mode::Cebo);
  assert(!modules.transition(Mode::Idle, 10004) && !radio.nat);
  ModuleOutput out;
  out.mode = Mode::Escudo;
  strcpy(out.name, "escudo.coverage");
  strcpy(out.severity, "info");
  strcpy(out.json,
         "{\"channel\":1,\"pause_reason\":\"none\",\"started_uptime_ms\":0,"
         "\"duration_ms\":1,\"dropped_frames\":0,\"dropped_events\":0}");
  ModuleOutputQueue outputs;
  assert(validate_output(out));
  for (unsigned i = 0; i < 8; ++i)
    assert(outputs.push(out));
  assert(!outputs.push(out) && outputs.dropped() == 1);
  strcpy(out.json, "{\"password\":\"should-never-be-persisted\"}");
  assert(!outputs.push(out) && outputs.dropped() == 2);
  for (unsigned i = 0; i < 8; ++i) {
    assert(outputs.pop(out));
    assert(!strstr(out.json, "password"));
  }
  assert(!outputs.pop(out));
  auto metrics = fixture("metrics/cebo");
  auto mj = v2::parse(metrics.c_str(), metrics.size());
  auto mp = cJSON_GetObjectItemCaseSensitive(mj, "payload");
  char *values =
      cJSON_PrintUnformatted(cJSON_GetObjectItemCaseSensitive(mp, "values"));
  out = {};
  out.metric = true;
  out.mode = Mode::Cebo;
  out.interval_ms = 1000;
  strcpy(out.json, values);
  cJSON_free(values);
  cJSON_Delete(mj);
  assert(outputs.push(out) && outputs.pop(out));
  out.interval_ms = 0;
  assert(!outputs.push(out));
  assert(!modules.transition(Mode::Idle, 11000));
  std::cout << "module consumer, v2 gate/dispatch, exclusive leases, stop "
               "timeout, outputs: PASS\n";
  char wire[MaxMessage + 1];
  Envelope env = {"node-example", "boot-example", 1, 1000};
  env.version = 2;
  assert(serialize(envelope(env, "state",
                            module_state_payload(
                                runtime, modules, "advertising", "disconnected",
                                "disconnected", true, true, true)),
                   wire));
}

void module_wire_fixtures() {
  Radio radio;
  Context context(radio);
  Runtime runtime;
  ModuleSupervisor modules(runtime, radio);
  assert(modules.add(Mode::Cebo, test_consumer(), context));
  Envelope env = {"node-example", "boot-example", 1, 1000};
  env.version = 2;
  char wire[MaxMessage + 1];
  assert(serialize(envelope(env, "state",
                            module_state_payload(
                                runtime, modules, "advertising", "disconnected",
                                "disconnected", true, true, true)),
                   wire));
  std::cout << wire << '\n';
  assert(!modules.transition(Mode::Cebo, 1000));
  assert(serialize(envelope(env, "state",
                            module_state_payload(runtime, modules, "connected",
                                                 "disconnected", "disconnected",
                                                 true, true, true)),
                   wire));
  std::cout << wire << '\n';
  Result result;
  strcpy(result.id, "cmd-example");
  strcpy(result.body, "{\"kind\":\"job\",\"job_id\":\"cmd-example\"}");
  assert(serialize(envelope(env, "ack", module_ack_payload(result, "applied")),
                   wire));
  std::cout << wire << '\n';
  auto j = v2::parse(fixture("events/cebo.http-benign").c_str(),
                     fixture("events/cebo.http-benign").size());
  auto payload = cJSON_GetObjectItemCaseSensitive(j, "payload");
  auto detail = cJSON_GetObjectItemCaseSensitive(payload, "details");
  char *raw = cJSON_PrintUnformatted(detail);
  assert(
      serialize(envelope(env, "event",
                         module_event_payload(1, "cebo.interaction", Mode::Cebo,
                                              nullptr, raw, "info", true)),
                wire));
  std::cout << wire << '\n';
  cJSON_free(raw);
  cJSON_Delete(j);
}
