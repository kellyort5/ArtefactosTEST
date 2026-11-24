#include "include/cebo.hpp"
#include <cstdio>
#include <cstring>

namespace sentinel::cebo {

const char *service_text(Service s) {
  switch (s) {
  case Service::Starting:
    return "starting";
  case Service::Running:
    return "running";
  case Service::Error:
    return "error";
  case Service::Stopped:
    break;
  }
  return "stopped";
}

bool Cebo::init(ModuleContext &c) {
  // El núcleo llama a init con el dispositivo en Idle: no se toma la radio ni
  // se abre ningún socket aquí.
  context_ = &c;
  return true;
}

bool Cebo::start() {
  if (!context_)
    return false;
  active_ = true;
  stopping_ = false;
  wanted_ = AutoStartServices;
  for (auto &s : services_) {
    s.status = wanted_ ? Service::Starting : Service::Stopped;
    s.retry_at = 0;
  }
  head_failing_ = false;
  counters_ = Counters();
  bus_.open();
  return true;
}

bool Cebo::all_stopped() const {
  for (const auto &s : services_)
    if (!s.listener->stopped())
      return false;
  return true;
}

void Cebo::request_stop() {
  stopping_ = true;
  wanted_ = false;
  // Cerrar primero el bus: las desconexiones provocadas por la parada no
  // deben producir interacciones nuevas.
  bus_.close();
  finish_stop();
}

bool Cebo::stopped() const { return all_stopped(); }

void Cebo::finish_stop() {
  // Un cierre incompleto se reintenta en cada tick hasta el plazo del
  // supervisor; el módulo no queda libre hasta que los dos servicios cerraron.
  if (!all_stopped()) {
    for (auto &s : services_)
      if (!s.listener->stopped())
        s.listener->request_stop();
    if (!all_stopped())
      return;
  }
  active_ = false;
  stopping_ = false;
  for (auto &s : services_)
    s.status = Service::Stopped;
}

void Cebo::handle(const ModuleCommand &cmd, ModuleReply &reply) {
  if (!active_ || stopping_) {
    reply.error = "invalid_state";
    return;
  }
  if (!strcmp(cmd.operation, "cebo.start")) {
    if (!wanted_) {
      bus_.open();
      wanted_ = true;
    }
  } else if (!strcmp(cmd.operation, "cebo.stop")) {
    wanted_ = false;
    bus_.close();
    for (auto &s : services_)
      s.listener->request_stop();
  } else {
    // cebo.configure y cebo.stats pertenecen al cambio de eventos e historial.
    reply.error = "not_implemented";
  }
}

void Cebo::drain(uint64_t now_ms) {
  for (unsigned i = 0; i < EventsPerTick; ++i) {
    Interaction in;
    if (!bus_.front(in))
      return;
    if (publishable(in)) {
      char details[320];
      if (format_details(in, details, sizeof(details))) {
        if (!context_->emit_event("cebo.interaction", "info", details,
                                  false)) {
          // Salida llena o rechazada: se conserva y se reintenta. Si no se
          // acepta durante EventRetryMs se descarta para no bloquear el bus.
          if (!head_failing_) {
            head_failing_ = true;
            head_failing_since_ = now_ms;
          }
          if (now_ms - head_failing_since_ < EventRetryMs)
            return;
          ++counters_.publish_failed;
        } else {
          ++counters_.published;
        }
      } else {
        ++counters_.publish_failed;
      }
    }
    head_failing_ = false;
    ++counters_.interactions[unsigned(in.kind)];
    bus_.pop();
  }
}

void Cebo::step(Control &s, bool lan, uint64_t now_ms) {
  if (s.status == Service::Running) {
    if (!lan) {
      // Sin LAN no hay sesiones útiles: cerrar y volver a arrancar al regresar.
      s.listener->request_stop();
      s.status = Service::Starting;
    }
    return;
  }
  if (s.status == Service::Stopped)
    s.status = Service::Starting;
  if (!s.listener->stopped()) {
    s.listener->request_stop();
    return;
  }
  if (lan && now_ms >= s.retry_at) {
    if (s.listener->start(bus_, ServiceSettings())) {
      s.status = Service::Running;
    } else {
      s.status = Service::Error;
      s.retry_at = now_ms + ServiceRetryMs;
    }
  }
}

void Cebo::tick(uint64_t now_ms) {
  if (!active_)
    return;
  if (stopping_) {
    finish_stop();
    return;
  }
  for (auto &s : services_)
    s.listener->tick();
  if (!wanted_) {
    for (auto &s : services_) {
      if (!s.listener->stopped())
        s.listener->request_stop();
      else
        s.status = Service::Stopped;
    }
    drain(now_ms);
    return;
  }
  bool lan = platform_.lan_ready();
  // Cada servicio arranca y se recupera por separado: un fallo de uno no
  // detiene al otro.
  for (auto &s : services_)
    step(s, lan, now_ms);
  drain(now_ms);
}

void Cebo::snapshot(ModuleSnapshot &out) const {
  unsigned connections = services_[0].listener->active_connections() +
                         services_[1].listener->active_connections();
  if (connections > MaxConnections)
    connections = MaxConnections;
  Service http = active_ ? services_[0].status : Service::Stopped;
  Service telnet = active_ ? services_[1].status : Service::Stopped;
  snprintf(out.state, sizeof(out.state),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":true,\"status\":\"%s\"},"
           "\"active_connections\":%u,\"max_connections\":%u,"
           "\"idle_timeout_ms\":%u}",
           service_text(http), service_text(telnet), connections,
           MaxConnections, unsigned(IdleTimeoutMs));
  out.capabilities = {};
  out.capabilities.honeypot_http = true;
  out.capabilities.honeypot_telnet = true;
  out.target_selected = false;
}

} // namespace sentinel::cebo
