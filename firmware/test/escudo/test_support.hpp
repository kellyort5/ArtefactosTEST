#pragma once
#include "capture_ingress.h"
#include "escudo.hpp"
#include "module_runtime.h"
#include "protocol_v2.h"
#include "wire.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
using namespace sentinel;
using namespace sentinel::escudo;
namespace sentinel {
uint64_t monotonic_ms() { return 1000; }
} // namespace sentinel
inline bool emit_wire = false;
inline const char *text(const cJSON *j, const char *key) { return str(j, key); }
static ScanNetwork network(unsigned id, const char *ssid = "LAB",
                           uint8_t channel = 6) {
  ScanNetwork n;
  snprintf(n.ssid, sizeof(n.ssid), "%s", ssid);
  n.bssid[0] = 2;
  n.bssid[4] = id >> 8;
  n.bssid[5] = id;
  n.channel = channel;
  n.rssi = -55;
  n.auth = 3;
  n.pairwise_cipher = n.group_cipher = 4;
  return n;
}
struct Radio : Resources, ModuleResources {
  CaptureIngress ingress;
  bool capture_ok = true, capture_stop_ok = true, physical_capture = false;
  unsigned capture_calls = 0;
  uint16_t channels = 0x7ff;
  bool leased = false, scanning = false, scan_ok = true, read_ok = true,
       channel_ok = true;
  Mode owner = Mode::Idle;
  unsigned scan_calls = 0, channel_calls = 0, stops = 0;
  uint8_t channel = 0;
  std::vector<ScanNetwork> records;
  std::deque<RadioEvent> events;
  bool stop_network() override {
    ++stops;
    leased = scanning = false;
    owner = Mode::Idle;
    events.clear();
    return true;
  }
  bool network_released() const override { return !leased && !scanning; }
  bool start_cebo() override {
    assert(false);
    return false;
  }
  bool start_profile(Mode m) override {
    assert(network_released());
    owner = m;
    leased = m != Mode::Idle;
    return true;
  }
  bool set_channel(uint8_t c) override {
    assert(leased && owner == Mode::Escudo);
    ++channel_calls;
    if (!channel_ok)
      return false;
    channel = c;
    return true;
  }
  bool scan_passive() override {
    assert(leased && owner == Mode::Escudo);
    ++scan_calls;
    if (!scan_ok)
      return false;
    scanning = true;
    return true;
  }
  size_t scan_results(ScanNetwork *out, size_t capacity) override {
    size_t count = 0;
    return read_scan_results(out, capacity, count) ? count : 0;
  }
  bool read_scan_results(ScanNetwork *out, size_t capacity,
                         size_t &count) override {
    count = 0;
    if (!read_ok)
      return false;
    count = std::min(records.size(), capacity);
    for (size_t i = 0; i < count; ++i)
      out[i] = records[i];
    return true;
  }
  bool capture(bool enabled) override {
    ++capture_calls;
    if (enabled) {
      if (!capture_ok)
        return false;
      physical_capture = true;
      ingress.open();
      return true;
    }
    ingress.close();
    if (!capture_stop_ok)
      return false;
    physical_capture = false;
    return true;
  }
  uint32_t capture_generation() const override { return ingress.generation(); }
  uint64_t capture_dropped() const override { return ingress.dropped(); }
  uint16_t allowed_channels() const override { return channels; }
  void feed(const RadioEvent &raw) {
    RadioEvent copied;
    if (!ingress.copy(raw.bytes, raw.length, true, raw.channel, raw.rssi,
                      raw.uptime_ms, copied))
      return;
    if (events.size() >= RadioQueueSize)
      ingress.drop();
    else
      events.push_back(copied);
  }
  bool configure_upstream(const UpstreamConfig &) override {
    assert(false);
    return false;
  }
  bool configure_ap(const ApConfig &) override {
    assert(false);
    return false;
  }
  bool napt(bool) override {
    assert(false);
    return false;
  }
  esp_netif_obj *sta_netif() const override { return nullptr; }
  esp_netif_obj *ap_netif() const override { return nullptr; }
  bool poll(RadioEvent &e) override {
    if (events.empty())
      return false;
    e = events.front();
    events.pop_front();
    return true;
  }
  uint64_t dropped() const override { return 0; }
  void done(uint32_t status = 0) {
    assert(events.size() < 8);
    RadioEvent e = {};
    e.kind = RadioEvent::Kind::ScanComplete;
    e.scan_status = status;
    e.scan_count = records.size();
    e.uptime_ms = 1001;
    scanning = false;
    events.push_back(e);
  }
};
struct Context : ModuleContext {
  ModuleLease lease;
  uint64_t now = 1000, seq = 0;
  bool sink_ready = true;
  std::vector<std::string> events;
  explicit Context(ModuleSupervisor &s, Radio &r) : lease(s, Mode::Escudo, r) {}
  uint64_t now_ms() const override { return now; }
  ModuleResources &resources() override { return lease; }
  bool emit_event(const char *name, const char *severity, const char *details,
                  bool simulated) override {
    if (!sink_ready)
      return false;
    ModuleOutput out;
    out.mode = Mode::Escudo;
    assert(!simulated); // Production module consumes real radio; this sink uses
                        // host fixtures.
    out.simulated = true;
    strcpy(out.name, name);
    strcpy(out.severity, severity);
    strcpy(out.json, details);
    assert(validate_output(out));
    Envelope env = {"node-test", "boot-test", ++seq, now};
    env.version = 2;
    char wire[MaxMessage + 1];
    assert(serialize(
        envelope(env, "event",
                 module_event_payload(seq, name, Mode::Escudo, nullptr, details,
                                      severity, true)),
        wire));
    assert(events.size() < 64);
    events.emplace_back(wire);
    if (emit_wire)
      std::cout << wire << '\n';
    return true;
  }
  bool publish_metrics(uint32_t interval, const char *values) override {
    ModuleOutput out;
    out.mode = Mode::Escudo;
    out.metric = true;
    out.interval_ms = interval;
    strcpy(out.json, values);
    assert(validate_output(out));
    Envelope env = {"node-test", "boot-test", ++seq, now};
    env.version = 2;
    char wire[MaxMessage + 1];
    auto payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "scope", "escudo");
    cJSON_AddNumberToObject(payload, "interval_ms", interval);
    cJSON_AddItemToObject(payload, "values", cJSON_Parse(values));
    assert(serialize(envelope(env, "metrics", payload), wire));
    if (emit_wire)
      std::cout << wire << '\n';
    return true;
  }
};
struct Harness {
  Radio radio;
  Runtime runtime;
  ModuleSupervisor supervisor{runtime, radio};
  Context context{supervisor, radio};
  Escudo module;
  CommandGate gate;
  unsigned command_sequence = 0;
  Harness() {
    assert(supervisor.add(Mode::Escudo, module, context));
    assert(!supervisor.transition(Mode::Escudo, context.now));
  }
  std::string command_text(const char *operation, const char *args,
                           const char *id) {
    auto p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "command_id", id);
    cJSON_AddStringToObject(p, "target_boot_id", "boot-test");
    cJSON_AddNumberToObject(p, "expected_revision", runtime.revision);
    cJSON_AddNumberToObject(p, "expires_at_uptime_ms", context.now + 30000);
    cJSON_AddStringToObject(p, "operation", operation);
    cJSON_AddItemToObject(p, "args", cJSON_Parse(args));
    Envelope env = {"node-test", "boot-test", ++context.seq, context.now};
    env.version = 2;
    char wire[MaxMessage + 1];
    assert(serialize(envelope(env, "command", p), wire));
    return wire;
  }
  Result send(const std::string &data, bool authorized = true) {
    Command c;
    assert(!c.parse_v2(data.c_str(), data.size()));
    uint8_t digest[32];
    assert(digest_command(c, digest));
    const Result *cached = nullptr;
    Result result;
    strcpy(result.id, c.id);
    result.revision = runtime.revision;
    result.error = gate.check(
        c, "node-test", "boot-test", runtime.revision, context.now, authorized,
        false, digest, &cached,
        v2::admission(c, Channel::Ble, runtime.profile, true, false));
    if (!result.error && cached)
      result = *cached;
    else if (!result.error) {
      ModuleReply reply;
      result.error = supervisor.dispatch(c, reply);
      if (!result.error) {
        if (reply.job)
          snprintf(result.body, sizeof(result.body),
                   "{\"kind\":\"job\",\"job_id\":\"%s\"}", c.id);
        else
          strcpy(result.body, reply.result);
      }
      gate.remember(c, digest, result);
    }
    Envelope env = {"node-test", "boot-test", ++context.seq, context.now};
    env.version = 2;
    char wire[MaxMessage + 1];
    assert(
        serialize(envelope(env, "ack",
                           module_ack_payload(
                               result, result.error ? "rejected" : "applied")),
                  wire));
    if (emit_wire) {
      std::cout << data << '\n' << wire << '\n';
    }
    return result;
  }
  Result command(const char *op, const char *args = "{}") {
    char id[40];
    snprintf(id, sizeof(id), "command-%u", ++command_sequence);
    return send(command_text(op, args, id));
  }
  std::string state() {
    Envelope env = {"node-test", "boot-test", ++context.seq, context.now};
    env.version = 2;
    char wire[MaxMessage + 1];
    assert(serialize(
        envelope(env, "state",
                 module_state_payload(runtime, supervisor, "connected",
                                      "disconnected", "disconnected", true,
                                      true, true)),
        wire));
    if (emit_wire)
      std::cout << wire << '\n';
    return wire;
  }
  void tick() { supervisor.tick(context.now); }
};
