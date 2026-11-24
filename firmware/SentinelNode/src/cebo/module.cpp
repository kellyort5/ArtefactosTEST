#include "include/cebo.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sentinel::cebo {
namespace {
const cJSON *member(const cJSON *object, const char *name) {
  return cJSON_GetObjectItemCaseSensitive(object, name);
}
// Entero no negativo exacto dentro de [low, high]; rechaza fracciones, NaN y
// cualquier tipo que no sea número.
bool whole(const cJSON *item, double low, double high, uint32_t &out) {
  if (!cJSON_IsNumber(item))
    return false;
  const double v = item->valuedouble;
  if (!(v >= low && v <= high) || v != std::floor(v))
    return false;
  out = uint32_t(v);
  return true;
}
// cebo.configure: HTTP y Telnet permanecen activos (el contrato no permite
// quitarlos); solo varían el máximo de conexiones y la inactividad.
bool parse_configure(const cJSON *args, Config &out) {
  if (!args || !cJSON_IsObject(args))
    return false;
  if (!cJSON_IsTrue(member(args, "http_enabled")) ||
      !cJSON_IsTrue(member(args, "telnet_enabled")))
    return false;
  uint32_t max = 0, idle = 0;
  if (!whole(member(args, "max_connections"), 1, MaxConnections, max) ||
      !whole(member(args, "idle_timeout_ms"), MinIdleTimeoutMs,
             MaxIdleTimeoutMs, idle))
    return false;
  out.max_connections = max;
  out.idle_timeout_ms = idle;
  return valid(out);
}
} // namespace

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
  counters_ = Counters();
  pending_ = Pending();
  classifier_.reset();
  metrics_.reset();
  // La configuración aceptada se restaura al entrar en el modo, sin tocar la
  // radio ni abrir sockets por sí misma. Un registro dañado se informa y se
  // usan los valores por defecto; nunca se borra ni se formatea nada.
  config_ = Config();
  storage_error_ = false;
  Config restored;
  stored_ok_ = false;
  switch (platform_.config().load(restored)) {
  case Load::Loaded:
    config_ = restored;
    stored_ok_ = true;
    break;
  case Load::Error:
    storage_error_ = true;
    break;
  case Load::Missing:
    break;
  }
  limiter_.configure(config_);
  limiter_.clear();
  last_metrics_ = context_->now_ms();
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
  pending_ = Pending();
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

void Cebo::configure(const ModuleCommand &cmd, ModuleReply &reply) {
  Config next;
  if (!parse_configure(cmd.args, next)) {
    reply.error = "invalid_args";
    return;
  }
  // Ya vigente y confirmada en flash: no se desgasta. Si el registro faltaba o
  // estaba dañado, una selección explícita válida lo escribe aunque coincida
  // con los valores por defecto.
  if (next == config_ && stored_ok_)
    return;
  // La configuración solo se confirma tras el commit; ante un fallo se
  // conserva la anterior, también en RAM.
  if (!platform_.config().save(next)) {
    reply.error = "resource_error";
    return;
  }
  config_ = next;
  stored_ok_ = true;
  limiter_.configure(next);
}

void Cebo::stats(ModuleReply &reply) {
  const uint64_t now = context_->now_ms();
  char values[512];
  if (!metrics_.format(values, sizeof(values),
                       services_[0].listener->active_connections(),
                       services_[1].listener->active_connections(),
                       limiter_.rejected(), now)) {
    reply.error = "resource_error";
    return;
  }
  uint64_t interval = now > last_metrics_ ? now - last_metrics_ : 1;
  if (interval > UINT32_MAX)
    interval = UINT32_MAX;
  int n = snprintf(reply.result, sizeof(reply.result),
                   "{\"kind\":\"cebo.stats\",\"interval_ms\":%lu,\"values\":%s}",
                   (unsigned long)interval, values);
  if (n <= 0 || size_t(n) >= sizeof(reply.result)) {
    *reply.result = 0;
    reply.error = "resource_error";
  }
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
  } else if (!strcmp(cmd.operation, "cebo.configure")) {
    configure(cmd, reply);
  } else if (!strcmp(cmd.operation, "cebo.stats")) {
    stats(reply);
  } else {
    reply.error = "not_implemented";
  }
}

// Entrega el evento pendiente. Verdadero si ya no queda ninguno retenido: se
// aceptó o se descartó tras EventRetryMs para no bloquear la cola.
bool Cebo::deliver(uint64_t now_ms) {
  if (!pending_.used)
    return true;
  const Classification &c = pending_.classification;
  if (context_->emit_event("cebo.interaction", severity_text(c.suspicion),
                           pending_.details, false)) {
    ++counters_.published;
    classifier_.commit(pending_.interaction, c, true);
    if (c.suspicion != Suspicion::Informational)
      metrics_.alert(pending_.interaction.protocol);
    pending_ = Pending();
    return true;
  }
  // Salida llena o rechazada: se conserva y se reintenta.
  if (!pending_.failing) {
    pending_.failing = true;
    pending_.since = now_ms;
  }
  if (now_ms - pending_.since < EventRetryMs)
    return false;
  ++counters_.publish_failed;
  classifier_.commit(pending_.interaction, c, false);
  pending_ = Pending();
  return true;
}

void Cebo::drain(uint64_t now_ms) {
  unsigned budget = EventsPerTick;
  // Cada vuelta consume interacciones hasta encontrar una que produzca evento;
  // las que solo suman contadores no gastan el presupuesto de eventos.
  for (unsigned guard = 0; budget && guard < BusCapacity * 2; ++guard) {
    if (!deliver(now_ms))
      return; // el evento retenido sigue esperando al núcleo
    Interaction in;
    if (!bus_.front(in))
      return;
    bus_.pop();
    ++counters_.interactions[unsigned(in.kind)];
    metrics_.record(in);
    Classification c = classifier_.observe(in);
    if (!c.publish)
      continue;
    Pending next;
    next.used = true;
    next.interaction = in;
    next.classification = c;
    if (!format_details(in, c, next.details, sizeof(next.details))) {
      ++counters_.publish_failed;
      continue;
    }
    pending_ = next;
    if (deliver(now_ms))
      --budget;
    else
      return;
  }
}

void Cebo::report_storage_error() {
  if (storage_error_ &&
      context_->emit_event(
          "system.status", "warning",
          "{\"status_code\":\"resource_error\",\"resource\":\"system\"}"))
    storage_error_ = false;
}

void Cebo::publish_metrics(uint64_t now_ms) {
  if (now_ms <= last_metrics_ || now_ms - last_metrics_ < MetricsIntervalMs)
    return;
  char values[512];
  uint64_t interval = now_ms - last_metrics_;
  if (interval > UINT32_MAX)
    interval = UINT32_MAX;
  if (metrics_.format(values, sizeof(values),
                      services_[0].listener->active_connections(),
                      services_[1].listener->active_connections(),
                      limiter_.rejected(), now_ms) &&
      context_->publish_metrics(uint32_t(interval), values))
    ++counters_.metrics_published;
  // Una muestra rechazada se omite: la siguiente llega en MetricsIntervalMs.
  last_metrics_ = now_ms;
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
    ServiceSettings settings;
    settings.limiter = &limiter_;
    if (s.listener->start(bus_, settings)) {
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
  classifier_.expire(now_ms);
  for (auto &s : services_)
    s.listener->tick();
  if (!wanted_) {
    for (auto &s : services_) {
      if (!s.listener->stopped())
        s.listener->request_stop();
      else
        s.status = Service::Stopped;
    }
  } else {
    bool lan = platform_.lan_ready();
    // Cada servicio arranca y se recupera por separado: un fallo de uno no
    // detiene al otro.
    for (auto &s : services_)
      step(s, lan, now_ms);
  }
  drain(now_ms);
  report_storage_error();
  publish_metrics(now_ms);
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
           config_.max_connections, unsigned(config_.idle_timeout_ms));
  out.capabilities = {};
  out.capabilities.honeypot_http = true;
  out.capabilities.honeypot_telnet = true;
  out.target_selected = false;
}

} // namespace sentinel::cebo
