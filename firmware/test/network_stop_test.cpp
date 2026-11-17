#include "../SentinelNode/src/sentinel/include/module_runtime.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace sentinel {
uint64_t monotonic_ms() { return 0; }
}

namespace {
using namespace sentinel;

// Aísla la liberación de los recursos de Cebo después de parar sus servicios.
struct PendingRadio final : Resources {
  enum class Result { Pending, Complete, Failed, Inconsistent };
  Result result = Result::Pending;
  bool held = true;
  unsigned starts = 0;

  bool stop_network() override {
    if (result == Result::Complete)
      held = false;
    return result == Result::Complete || result == Result::Inconsistent;
  }
  bool network_stopping() const override { return result == Result::Pending; }
  bool network_released() const override { return !held; }
  bool start_cebo() override { return start_profile(Profile::Cebo); }
  bool start_profile(Profile target) override {
    assert(!held); // Nunca adquirir otro perfil con el recurso anterior vivo.
    ++starts;
    held = target != Profile::Idle;
    return true;
  }
};

struct Fixture {
  Runtime state;
  PendingRadio radio;
  ModuleSupervisor modules;
  Fixture() : modules(state, radio) {
    state.profile = Profile::Cebo;
    state.phase = "ready";
  }
  void assert_error(const char *expected) const {
    assert(!modules.transitioning());
    assert(!strcmp(state.phase, "error") && state.error);
    assert(!strcmp(state.error, expected));
    assert(state.profile == Profile::Cebo && state.revision == 0);
    assert(radio.held && radio.starts == 0);
  }
};
} // namespace

int main() {
  using Result = PendingRadio::Result;
  {
    Fixture f;
    assert(!f.modules.transition(Profile::Idle, 100));
    assert(f.modules.transitioning() && !f.state.error);
    assert(f.state.profile == Profile::Cebo && f.state.revision == 0);
    assert(f.radio.held && f.radio.starts == 0);
    assert(!strcmp(f.modules.transition(Profile::Idle, 200), "busy"));
    f.modules.tick(4000);
    assert(f.modules.transitioning() && f.radio.starts == 0);
    f.radio.result = Result::Complete;
    f.modules.tick(5000);
    assert(!f.modules.transitioning() && !f.state.error);
    assert(f.state.profile == Profile::Idle && f.state.revision == 1);
    assert(!f.radio.held && f.radio.starts == 1);
  }
  {
    Fixture f;
    assert(!f.modules.transition(Profile::Idle, 100));
    f.modules.tick(5099);
    assert(f.modules.transitioning());
    f.modules.tick(5100); // El pendiente no reinicia ni amplía los 5000 ms.
    f.assert_error("timeout");
    f.radio.result = Result::Complete;
    f.modules.tick(5200); // Un ACK tardío no aplica una orden ya fallida.
    f.assert_error("timeout");
    assert(!f.modules.transition(Profile::Idle, 5201));
    assert(!f.modules.transitioning() && f.state.profile == Profile::Idle);
    assert(f.state.revision == 1 && !f.radio.held && f.radio.starts == 1);
  }
  for (auto result : {Result::Failed, Result::Inconsistent}) {
    Fixture f;
    f.radio.result = result;
    assert(!strcmp(f.modules.transition(Profile::Idle, 100), "resource_error"));
    f.assert_error("resource_error");
  }
  {
    Fixture f;
    assert(!f.modules.transition(Profile::Idle, 100));
    f.radio.result = Result::Failed;
    f.modules.tick(101);
    f.assert_error("resource_error");
  }
  {
    // Los adaptadores previos no necesitan implementar la nueva señal.
    struct LegacyRadio final : Resources {
      bool stop_network() override { return false; }
      bool network_released() const override { return false; }
      bool start_cebo() override { return false; }
    } radio;
    Runtime state;
    state.profile = Profile::Cebo;
    ModuleSupervisor modules(state, radio);
    assert(!radio.network_stopping());
    assert(!strcmp(modules.transition(Profile::Idle, 100), "resource_error"));
    assert(!modules.transitioning() && state.profile == Profile::Cebo);
  }
  std::cout << "Pending network stop, ACK, timeout, errors and Idle retry: PASS (host)\n";
}
