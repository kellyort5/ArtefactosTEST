#pragma once
// Dobles y utilidades compartidas por las pruebas de host de Cebo.
#include "../../SentinelNode/src/cebo/include/cebo.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace sentinel;
using namespace sentinel::cebo;

void events_tests(); // events_test.cpp
void events_dump();

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,             \
                   #condition);                                                \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)


// ---- Dobles para el ciclo de vida -----------------------------------------
struct FakeResources : ModuleResources {
  bool set_channel(uint8_t) override { return false; }
  bool scan_passive() override { return false; }
  size_t scan_results(ScanNetwork *, size_t) override { return 0; }
  bool capture(bool) override { return false; }
  bool configure_upstream(const UpstreamConfig &) override { return false; }
  bool configure_ap(const ApConfig &) override { return false; }
  bool napt(bool) override { return false; }
  esp_netif_obj *sta_netif() const override { return nullptr; }
  esp_netif_obj *ap_netif() const override { return nullptr; }
  bool poll(RadioEvent &) override { return false; }
  uint64_t dropped() const override { return 0; }
};
struct Emitted {
  std::string name, severity, details;
  bool simulated;
};
struct FakeContext : ModuleContext {
  FakeResources resources_;
  uint64_t now = 1000;
  bool accept = true;
  std::vector<Emitted> events;
  uint64_t now_ms() const override { return now; }
  ModuleResources &resources() override { return resources_; }
  bool emit_event(const char *name, const char *severity, const char *details,
                  bool simulated) override {
    if (!accept)
      return false;
    events.push_back({name, severity, details, simulated});
    return true;
  }
  struct Metric {
    uint32_t interval_ms;
    std::string values;
  };
  std::vector<Metric> metrics;
  bool accept_metrics = true;
  bool publish_metrics(uint32_t interval, const char *values) override {
    if (!accept_metrics)
      return false;
    metrics.push_back({interval, values});
    return true;
  }
};
struct FakeListener : ServiceListener {
  bool start_ok = true, running = false, stuck = false;
  unsigned starts = 0, stops = 0, ticks = 0, connections = 0;
  InteractionBus *bus = nullptr;
  Limiter *limiter = nullptr;
  bool start(InteractionBus &b, const ServiceSettings &settings) override {
    ++starts;
    if (!start_ok)
      return false;
    CHECK(!running); // el módulo nunca arranca un listener ya abierto
    CHECK(settings.limiter); // los dos servicios comparten el límite del módulo
    limiter = settings.limiter;
    running = true;
    bus = &b;
    return true;
  }
  void request_stop() override {
    ++stops;
    if (!stuck)
      running = false;
  }
  bool stopped() const override { return !running; }
  unsigned active_connections() const override { return connections; }
  void tick() override { ++ticks; }
};
// Almacén en memoria con inyección de fallos: save solo cambia lo almacenado
// cuando devuelve verdadero (confirmación tras commit).
struct FakeStore : ConfigStore {
  Load load_result = Load::Missing;
  Config stored;
  unsigned loads = 0, saves = 0;
  bool save_ok = true;
  Load load(Config &out) override {
    ++loads;
    if (load_result == Load::Loaded)
      out = stored;
    return load_result;
  }
  bool save(const Config &c) override {
    ++saves;
    if (!save_ok)
      return false;
    stored = c;
    load_result = Load::Loaded;
    return true;
  }
};
struct FakeCebo : CeboPlatform {
  bool lan = false;
  FakeListener h, t;
  FakeStore store;
  bool lan_ready() override { return lan; }
  ServiceListener &http() override { return h; }
  ServiceListener &telnet() override { return t; }
  ConfigStore &config() override { return store; }
};
[[maybe_unused]] static std::string state_of(const Cebo &cebo) {
  ModuleSnapshot s;
  cebo.snapshot(s);
  return s.state;
}
[[maybe_unused]] static Interaction make(Kind kind, Path path = Path::None) {
  Interaction i;
  i.ipv4[0] = 192;
  i.ipv4[2] = 2;
  i.ipv4[3] = 2;
  i.source_port = 45000;
  i.destination_port = 80;
  i.kind = kind;
  i.path = path;
  return i;
}
[[maybe_unused]] static Interaction make_telnet(Kind kind, Close close = Close::None) {
  Interaction i = make(kind);
  i.protocol = Protocol::Telnet;
  i.destination_port = 23;
  i.close = close;
  return i;
}
[[maybe_unused]] static std::string state_text(const char *http, const char *telnet,
                              unsigned connections) {
  char text[420];
  snprintf(text, sizeof(text),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":true,\"status\":\"%s\"},"
           "\"active_connections\":%u,\"max_connections\":6,"
           "\"idle_timeout_ms\":5000}",
           http, telnet, connections);
  return text;
}

