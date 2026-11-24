#pragma once
#include "core.h"
#include "../../sentinel_api/include/module_api.hpp"
#include <atomic>
namespace sentinel {
class ModuleSupervisor {
  Runtime &state;
  Resources &radio;
  Module *entries[4] = {};
  Profile target = Profile::Idle;
  uint64_t deadline = 0;
  bool moving = false;
  std::atomic<Profile> lease_owner{Profile::Idle};
  bool finish();

public:
  ModuleSupervisor(Runtime &s, Resources &r) : state(s), radio(r) {}
  bool add(Profile, Module &, ModuleContext &);
  Module *get(Profile p) const {
    return static_cast<unsigned>(p) < 4 ? entries[static_cast<unsigned>(p)]
                                        : nullptr;
  }
  bool available(Profile p) const { return p == Profile::Idle || get(p); }
  bool active(Profile p) const { return lease_owner.load() == p && get(p); }
  bool transitioning() const { return moving; }
  const char *transition(Profile, uint64_t now);
  void tick(uint64_t now);
  Profile next() const;
  const char *dispatch(const Command &, ModuleReply &);
  ModuleCapabilities capabilities() const;
};
// Cada módulo conserva su lease: retener un contexto antiguo nunca concede
// la radio de otro modo. Solo el supervisor usa estos métodos; los hooks de
// salida copian datos a su cola acotada.
class ModuleLease : public ModuleResources {
  const ModuleSupervisor &supervisor;
  Profile mode;
  ModuleResources &radio;
  bool allowed() const { return supervisor.active(mode); }

public:
  ModuleLease(const ModuleSupervisor &s, Profile p, ModuleResources &r)
      : supervisor(s), mode(p), radio(r) {}
  bool set_channel(uint8_t c) override {
    return allowed() && radio.set_channel(c);
  }
  bool scan_passive() override { return allowed() && radio.scan_passive(); }
  size_t scan_results(ScanNetwork *p, size_t n) override {
    return allowed() ? radio.scan_results(p, n) : 0;
  }
  bool read_scan_results(ScanNetwork *out, size_t capacity,
                         size_t &count) override {
    count = 0;
    return allowed() && radio.read_scan_results(out, capacity, count);
  }
  bool capture(bool b) override { return allowed() && radio.capture(b); }
  uint32_t capture_generation() const override {
    return allowed() ? radio.capture_generation() : 0;
  }
  uint64_t capture_dropped() const override {
    return allowed() ? radio.capture_dropped() : 0;
  }
  uint16_t allowed_channels() const override {
    return allowed() ? radio.allowed_channels() : 0;
  }
  bool configure_upstream(const UpstreamConfig &c) override {
    return allowed() && radio.configure_upstream(c);
  }
  bool configure_ap(const ApConfig &c) override {
    return allowed() && radio.configure_ap(c);
  }
  bool napt(bool b) override {
    return allowed() && mode == Profile::Sanitizador && radio.napt(b);
  }
  esp_netif_obj *sta_netif() const override {
    return allowed() ? radio.sta_netif() : nullptr;
  }
  esp_netif_obj *ap_netif() const override {
    return allowed() ? radio.ap_netif() : nullptr;
  }
  bool poll(RadioEvent &e) override { return allowed() && radio.poll(e); }
  uint64_t dropped() const override { return allowed() ? radio.dropped() : 0; }
};
struct ModuleOutput {
  Profile mode = Profile::Idle;
  bool metric = false, simulated = false;
  char name[40] = {}, severity[12] = {};
  uint32_t interval_ms = 0;
  char json[ModuleJsonBytes + 1] = {};
};
class ModuleOutputQueue {
  ModuleOutput items[QueueSize];
  size_t head = 0, count = 0;
  mutable std::atomic_flag lock = ATOMIC_FLAG_INIT;
  std::atomic<uint64_t> lost{0};

public:
  bool push(const ModuleOutput &);
  bool pop(ModuleOutput &);
  uint64_t dropped() const { return lost.load(); }
};
bool validate_output(const ModuleOutput &);
struct Envelope;
// Solo el supervisor persiste. El evento debe quedar durable antes de intentar
// entregarlo; los reintentos leen Journal::read sin volver a crear el evento.
const char *persist_module_event(Journal &, const ModuleOutput &,
                                 const Envelope &, char out[MaxMessage + 1]);
} // namespace sentinel
