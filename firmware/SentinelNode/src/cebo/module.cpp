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
  wanted_ = AutoStartHttp;
  status_ = wanted_ ? Service::Starting : Service::Stopped;
  retry_at_ = 0;
  head_failing_ = false;
  counters_ = Counters();
  bus_.open();
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

bool Cebo::stopped() const { return http_.stopped(); }

void Cebo::finish_stop() {
  // Un cierre fallido se reintenta en cada tick hasta el plazo del supervisor.
  if (!http_.stopped()) {
    http_.request_stop();
    if (!http_.stopped())
      return;
  }
  active_ = false;
  stopping_ = false;
  status_ = Service::Stopped;
}

void Cebo::handle(const ModuleCommand &cmd, ModuleReply &reply) {
  if (!active_ || stopping_) {
    reply.error = "invalid_state";
    return;
  }
  if (!strcmp(cmd.operation, "cebo.start")) {
    wanted_ = true;
  } else if (!strcmp(cmd.operation, "cebo.stop")) {
    wanted_ = false;
    bus_.close();
    http_.request_stop();
  } else {
    // cebo.configure y cebo.stats pertenecen a los cambios de Telnet y de
    // eventos/historial.
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

void Cebo::tick(uint64_t now_ms) {
  if (!active_)
    return;
  if (stopping_) {
    finish_stop();
    return;
  }
  http_.tick();
  if (!wanted_) {
    if (!http_.stopped())
      http_.request_stop();
    else
      status_ = Service::Stopped;
    drain(now_ms);
    return;
  }
  bool lan = http_.lan_ready();
  if (status_ == Service::Running) {
    if (!lan) {
      // Sin LAN no hay sesiones útiles: cerrar y volver a arrancar al regresar.
      http_.request_stop();
      status_ = Service::Starting;
    }
  } else {
    if (status_ == Service::Stopped)
      status_ = Service::Starting;
    if (!http_.stopped())
      http_.request_stop();
    else if (lan && now_ms >= retry_at_) {
      bus_.open();
      if (http_.start(bus_, HttpSettings())) {
        status_ = Service::Running;
      } else {
        status_ = Service::Error;
        retry_at_ = now_ms + HttpRetryMs;
      }
    }
  }
  drain(now_ms);
}

void Cebo::snapshot(ModuleSnapshot &out) const {
  unsigned connections = http_.active_connections();
  if (connections > 6)
    connections = 6;
  Service status = active_ ? status_ : Service::Stopped;
  snprintf(out.state, sizeof(out.state),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":false,\"status\":\"unavailable\"},"
           "\"active_connections\":%u,\"max_connections\":%u,"
           "\"idle_timeout_ms\":%u}",
           service_text(status), connections, unsigned(HttpMaxSockets),
           unsigned(HttpIdleTimeoutMs));
  out.capabilities = {};
  out.capabilities.honeypot_http = true;
  out.target_selected = false;
}

} // namespace sentinel::cebo
