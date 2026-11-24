// Clasificación de interacciones por origen y protocolo. Lógica pura: usa solo
// la marca de tiempo de la interacción, de modo que se prueba con un reloj
// controlado. Los contadores se actualizan siempre, aunque el evento se
// suprima por enfriamiento.
#include "include/cebo.hpp"
#include <cstring>

namespace sentinel::cebo {
namespace {
uint32_t plus_one(uint32_t value) { return value == UINT32_MAX ? value : value + 1; }
uint32_t saturating_sum(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  uint64_t sum = uint64_t(a) + b + c + d;
  return sum > UINT32_MAX ? UINT32_MAX : uint32_t(sum);
}
uint8_t level_of(Suspicion s) {
  return s == Suspicion::High ? 2 : s == Suspicion::Medium ? 1 : 0;
}
} // namespace

Classifier::Entry *Classifier::find(const Interaction &in) {
  for (auto &e : table_)
    if (e.used && e.protocol == in.protocol &&
        !memcmp(e.ipv4, in.ipv4, sizeof(e.ipv4)))
      return &e;
  return nullptr;
}

Classifier::Entry *Classifier::allocate(const Interaction &in) {
  Entry *slot = nullptr;
  for (auto &e : table_)
    if (!e.used) {
      slot = &e;
      break;
    }
  if (!slot) {
    // Tabla llena: se desaloja el origen visto hace más tiempo (LRU).
    slot = &table_[0];
    for (auto &e : table_)
      if (e.last_seen < slot->last_seen)
        slot = &e;
    evictions_ = plus_one(evictions_);
  }
  *slot = Entry();
  slot->used = true;
  memcpy(slot->ipv4, in.ipv4, sizeof(slot->ipv4));
  slot->protocol = in.protocol;
  slot->window_start = slot->last_seen = in.uptime_ms;
  return slot;
}

Classification Classifier::observe(const Interaction &in) {
  const uint64_t t = in.uptime_ms;
  expire(t);
  Entry *e = find(in);
  if (!e) {
    e = allocate(in);
  } else if (t >= e->window_start && t - e->window_start >= WindowMs) {
    // Ventana nueva: los conteos y el nivel ya publicado empiezan de cero.
    e->window_start = t;
    e->connections = e->accesses = e->auth_attempts = e->inputs = 0;
    e->alerted = 0;
  }
  if (t > e->last_seen)
    e->last_seen = t;
  switch (in.kind) {
  case Kind::Connect:
    e->connections = plus_one(e->connections);
    break;
  case Kind::Access:
    e->accesses = plus_one(e->accesses);
    break;
  case Kind::AuthAttempt:
    e->auth_attempts = plus_one(e->auth_attempts);
    break;
  case Kind::Input:
    e->inputs = plus_one(e->inputs);
    break;
  case Kind::Disconnect:
    break; // consecuencia de una conexión: no suma actividad
  }
  const uint32_t total =
      saturating_sum(e->connections, e->accesses, e->auth_attempts, e->inputs);

  Classification c;
  c.slot = unsigned(e - table_);
  c.evidence[0] = evidence_text(in);
  c.evidence_count = 1;
  if (total >= HighInteractions) {
    c.suspicion = Suspicion::High;
    c.count = total;
    c.window_ms = WindowMs;
    c.evidence[c.evidence_count++] = "rate_threshold";
  } else if (e->auth_attempts >= MediumAuthAttempts) {
    c.suspicion = Suspicion::Medium;
    c.count = e->auth_attempts;
    c.window_ms = WindowMs;
    c.evidence[c.evidence_count++] = "repeated_auth";
  }

  // Un origen publica como mucho un evento por CooldownMs, salvo cuando su
  // nivel sube respecto al último publicado en la ventana: la escalada es
  // información nueva. El cierre de conexión nunca produce evento.
  const bool escalation = level_of(c.suspicion) > e->alerted;
  // Un reloj que retrocede cuenta como dentro del enfriamiento.
  const bool cooled = !e->published || (t >= e->last_published &&
                                         t - e->last_published >= CooldownMs);
  if (in.kind == Kind::Disconnect)
    c.publish = false;
  else if (escalation)
    c.publish = true;
  else
    c.publish = publishable(in) && cooled;
  if (publishable(in) && !c.publish)
    suppressed_ = plus_one(suppressed_);
  return c;
}

void Classifier::commit(const Interaction &in, const Classification &c,
                        bool accepted) {
  if (!accepted || !c.publish || c.slot >= ClassifierEntries)
    return;
  Entry &e = table_[c.slot];
  // El origen pudo caducar o ser desalojado mientras el evento esperaba.
  if (!e.used || e.protocol != in.protocol ||
      memcmp(e.ipv4, in.ipv4, sizeof(e.ipv4)))
    return;
  e.published = true;
  e.last_published = in.uptime_ms;
  if (level_of(c.suspicion) > e.alerted)
    e.alerted = level_of(c.suspicion);
}

void Classifier::expire(uint64_t now_ms) {
  for (auto &e : table_)
    if (e.used && now_ms >= e.last_seen && now_ms - e.last_seen >= ExpiryMs)
      e = Entry();
}

void Classifier::reset() {
  for (auto &e : table_)
    e = Entry();
  suppressed_ = evictions_ = 0;
}

size_t Classifier::size() const {
  size_t n = 0;
  for (const auto &e : table_)
    n += e.used;
  return n;
}

} // namespace sentinel::cebo
