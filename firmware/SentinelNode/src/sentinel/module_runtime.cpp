#include "include/module_runtime.h"
#include "include/protocol_v2.h"
#include "include/wire.h"
#include <cstring>
namespace sentinel {
bool ModuleSupervisor::add(Profile p, Module &m, ModuleContext &ctx) {
  if (static_cast<unsigned>(p) >= 4 || p == Profile::Idle || get(p) || moving ||
      state.profile != Profile::Idle)
    return false;
  if (!m.init(ctx))
    return false;
  ModuleSnapshot snapshot;
  m.snapshot(snapshot);
  auto state = v2::parse(snapshot.state,
                         strnlen(snapshot.state, sizeof(snapshot.state)));
  const char *type = p == Profile::Escudo ? "V2EscudoModule"
                     : p == Profile::Cebo ? "V2CeboModule"
                                          : "V2SanitizadorModule";
  bool valid = state && v2::validate(type, state);
  cJSON_Delete(state);
  if (!valid)
    return false;
  entries[static_cast<unsigned>(p)] = &m;
  return true;
}
bool ModuleSupervisor::finish() {
  auto old = get(state.profile);
  if (old && !old->stopped())
    return false;
  const bool network_stopped = radio.stop_network();
  // Esperar el ACK del worker conserva el lease y el deadline de la transición.
  // Un fallo real o una liberación inconsistente no se ocultan como pendientes.
  if (!network_stopped && radio.network_stopping())
    return false;
  if (!network_stopped || !radio.network_released()) {
    state.phase = "error";
    state.error = "resource_error";
    moving = false;
    return false;
  }
  state.profile = Profile::Idle;
  lease_owner = Profile::Idle;
  auto m = get(target);
  // start() necesita su lease activo; ningún otro módulo recibe esta propiedad.
  state.profile = target;
  lease_owner = target;
  if (!radio.start_profile(target) || (m && !m->start())) {
    if (m)
      m->request_stop();
    // El módulo fallido conserva la propiedad hasta confirmar su parada;
    // otro modo no puede ocultar una tarea o socket parcialmente arrancado.
    state.phase = "error";
    state.error = "resource_error";
    moving = false;
    if (!m || m->stopped()) {
      radio.stop_network();
      if (radio.network_released()) {
        state.profile = Profile::Idle;
        lease_owner = Profile::Idle;
      }
    }
    return false;
  }
  moving = false;
  state.error = nullptr;
  state.phase = target == Profile::Idle ? "idle" : "ready";
  ++state.revision;
  return true;
}
const char *ModuleSupervisor::transition(Profile p, uint64_t now) {
  if (moving)
    return "busy";
  if (!available(p))
    return "not_implemented";
  if (!state.healthy && p != Profile::Idle)
    return "resource_error";
  if (!strcmp(state.phase, "error") && p != Profile::Idle)
    return "invalid_state";
  if (state.profile == p && !state.error)
    return nullptr;
  target = p;
  deadline = now + 5000;
  moving = true;
  state.phase = "transitioning";
  state.error = nullptr;
  if (auto old = get(state.profile))
    old->request_stop();
  finish();
  return moving ? nullptr : state.error;
}
void ModuleSupervisor::tick(uint64_t now) {
  if (auto m = get(state.profile))
    m->tick(now);
  if (!moving)
    return;
  if (now >= deadline) {
    moving = false;
    state.phase = "error";
    state.error = "timeout";
    return;
  }
  finish();
}
Profile ModuleSupervisor::next() const {
  const Profile order[] = {Profile::Escudo, Profile::Sanitizador,
                           Profile::Cebo};
  unsigned start = 0;
  for (unsigned i = 0; i < 3; ++i)
    if (order[i] == state.profile)
      start = i + 1;
  for (unsigned i = 0; i < 3; ++i)
    if (available(order[(start + i) % 3]))
      return order[(start + i) % 3];
  return Profile::Idle;
}
ModuleCapabilities ModuleSupervisor::capabilities() const {
  ModuleCapabilities c;
  for (auto m : entries)
    if (m) {
      ModuleSnapshot s;
      m->snapshot(s);
      c.capture |= s.capabilities.capture;
      c.detectors |= s.capabilities.detectors;
      c.honeypot_http |= s.capabilities.honeypot_http;
      c.honeypot_telnet |= s.capabilities.honeypot_telnet;
      c.notifications |= s.capabilities.notifications;
      c.sanitizador |= s.capabilities.sanitizador;
    }
  return c;
}
const char *ModuleSupervisor::dispatch(const Command &c, ModuleReply &reply) {
  if (moving)
    return "busy";
  Profile p = Profile::Idle;
  if (!strncmp(c.operation, "escudo.", 7))
    p = Profile::Escudo;
  else if (!strncmp(c.operation, "sanitizador.", 12))
    p = Profile::Sanitizador;
  else if (!strncmp(c.operation, "cebo.", 5))
    p = Profile::Cebo;
  auto m = get(p);
  if (!m)
    return "not_implemented";
  ModuleSnapshot s;
  m->snapshot(s);
  if (auto e = v2::new_context(c, state.profile, s.target_selected))
    return e;
  if (strcmp(state.phase, "ready"))
    return "invalid_state";
  // Las capacidades se comprueban antes de invocar código del módulo.
  if ((!strncmp(c.operation, "escudo.capture_", 15) &&
       !s.capabilities.capture) ||
      (!strncmp(c.operation, "sanitizador.gateway_", 20) &&
       !s.capabilities.sanitizador) ||
      (!strcmp(c.operation, "cebo.start") &&
       (!s.capabilities.honeypot_http || !s.capabilities.honeypot_telnet)))
    return "not_implemented";
  m->handle({c.operation, c.args, c.id}, reply);
  if (reply.error) {
    cJSON code = {};
    code.type = cJSON_String;
    code.valuestring = const_cast<char *>(reply.error);
    return v2::validate("V2ErrorCode", &code) ? reply.error : "resource_error";
  }
  cJSON *result = nullptr;
  if (reply.job) {
    result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "kind", "job");
    cJSON_AddStringToObject(result, "job_id", c.id);
  } else if (*reply.result)
    result =
        v2::parse(reply.result, strnlen(reply.result, sizeof(reply.result)));
  bool valid = (!*reply.result || result) && v2::result_for(c, result);
  cJSON_Delete(result);
  return valid ? nullptr : "invalid_message";
}
bool ModuleOutputQueue::push(const ModuleOutput &out) {
  // No esperamos al consumidor: perder una salida se contabiliza, bloquear
  // una tarea productora impediría conservar el control del dispositivo.
  if (!validate_output(out)) {
    ++lost;
    return false;
  }
  if (lock.test_and_set(std::memory_order_acquire)) {
    ++lost;
    return false;
  }
  bool ok = count < QueueSize;
  if (ok) {
    items[(head + count) % QueueSize] = out;
    ++count;
  }
  lock.clear(std::memory_order_release);
  if (!ok)
    ++lost;
  return ok;
}
bool ModuleOutputQueue::pop(ModuleOutput &out) {
  if (lock.test_and_set(std::memory_order_acquire))
    return false;
  bool ok = count > 0;
  if (ok) {
    out = items[head];
    head = (head + 1) % QueueSize;
    --count;
  }
  lock.clear(std::memory_order_release);
  return ok;
}
const char *persist_module_event(Journal &journal, const ModuleOutput &output,
                                 const Envelope &env, char json[MaxMessage + 1]) {
  json[0] = 0;
  if (output.metric)
    return "invalid_message";
  auto cursor = journal.highest + 1;
  if (!serialize(envelope(env, "event",
                          module_event_payload(cursor, output.name, output.mode,
                                               nullptr, output.json,
                                               output.severity, output.simulated)),
                 json))
    return "invalid_message";
  if (!journal.append(json, strlen(json), cursor)) {
    json[0] = 0;
    return "resource_error";
  }
  return nullptr;
}
bool validate_output(const ModuleOutput &out) {
  if (strnlen(out.name, sizeof(out.name)) == sizeof(out.name) ||
      strnlen(out.severity, sizeof(out.severity)) == sizeof(out.severity))
    return false;
  size_t n = strnlen(out.json, sizeof(out.json));
  if (n > ModuleJsonBytes)
    return false;
  auto detail = v2::parse(out.json, n);
  if (!detail)
    return false;
  auto p = cJSON_CreateObject();
  if (out.metric) {
    cJSON_AddStringToObject(p, "scope", profile_name(out.mode));
    cJSON_AddNumberToObject(p, "interval_ms", out.interval_ms);
    cJSON_AddItemToObject(p, "values", detail);
  } else {
    cJSON_AddNumberToObject(p, "journal_seq", 1);
    cJSON_AddStringToObject(p, "name", out.name);
    cJSON_AddStringToObject(p, "profile", profile_name(out.mode));
    cJSON_AddNullToObject(p, "code");
    cJSON_AddStringToObject(p, "origin", out.simulated ? "simulated" : "real");
    cJSON_AddStringToObject(p, "severity", out.severity);
    cJSON_AddItemToObject(p, "details", detail);
  }
  bool ok = v2::validate(out.metric ? "V2MetricsPayload" : "V2EventPayload", p);
  cJSON_Delete(p);
  return ok;
}
} // namespace sentinel
