// Contadores y métricas v2 de Cebo. Los contadores se actualizan con todas las
// interacciones, incluidas las que el clasificador no publica como evento.
#include "include/cebo.hpp"
#include <cstdio>
#include <cstring>

namespace sentinel::cebo {
namespace {
void bump(uint32_t &value) {
  if (value != UINT32_MAX)
    ++value;
}
} // namespace

void Metrics::reset() { *this = Metrics(); }

void Metrics::record(const Interaction &in) {
  Side &s = side_[idx(in.protocol)];
  switch (in.kind) {
  case Kind::Connect:
    bump(s.connections);
    break;
  case Kind::Access:
    bump(s.accesses);
    break;
  case Kind::AuthAttempt:
    bump(s.auth);
    break;
  case Kind::Input:
    bump(s.inputs);
    break;
  case Kind::Disconnect:
    return; // el cierre no es actividad del origen
  }
  // Ritmo: cubos de un segundo sobre los últimos 60 s.
  const uint64_t second = in.uptime_ms / 1000;
  const unsigned b = unsigned(second % Buckets);
  if (!s.bucket_used[b] || s.bucket_second[b] != second) {
    s.bucket_used[b] = true;
    s.bucket_second[b] = second;
    s.bucket_count[b] = 0;
  }
  bump(s.bucket_count[b]);
}

void Metrics::alert(Protocol p) { bump(side_[idx(p)].alerts); }

uint32_t Metrics::alerts() const {
  uint64_t sum = uint64_t(side_[0].alerts) + side_[1].alerts;
  return sum > UINT32_MAX ? UINT32_MAX : uint32_t(sum);
}

uint32_t Metrics::per_minute(Protocol p, uint64_t now_ms) const {
  const Side &s = side_[idx(p)];
  const uint64_t now = now_ms / 1000;
  uint64_t sum = 0;
  for (unsigned b = 0; b < Buckets; ++b)
    if (s.bucket_used[b] && s.bucket_second[b] + Buckets > now)
      sum += s.bucket_count[b];
  return sum > UINT32_MAX ? UINT32_MAX : uint32_t(sum);
}

bool Metrics::format(char *out, size_t capacity, unsigned http_active,
                     unsigned telnet_active, uint32_t rejected,
                     uint64_t now_ms) const {
  if (!out || !capacity)
    return false;
  *out = 0;
  const struct {
    Protocol protocol;
    unsigned active;
  } sides[2] = {{Protocol::Http, http_active}, {Protocol::Telnet, telnet_active}};
  char part[2][200];
  for (unsigned n = 0; n < 2; ++n) {
    const Side &s = side_[idx(sides[n].protocol)];
    unsigned active = sides[n].active > MaxConnections ? MaxConnections
                                                       : sides[n].active;
    int w = snprintf(part[n], sizeof(part[n]),
                     "{\"connections\":%lu,\"accesses\":%lu,"
                     "\"auth_attempts\":%lu,\"inputs\":%lu,"
                     "\"active_connections\":%u,"
                     "\"interactions_per_minute\":%lu,\"alerts\":%lu}",
                     (unsigned long)s.connections, (unsigned long)s.accesses,
                     (unsigned long)s.auth, (unsigned long)s.inputs, active,
                     (unsigned long)per_minute(sides[n].protocol, now_ms),
                     (unsigned long)s.alerts);
    if (w <= 0 || size_t(w) >= sizeof(part[n]))
      return false;
  }
  int n = snprintf(out, capacity,
                   "{\"http\":%s,\"telnet\":%s,\"rejected_connections\":%lu,"
                   "\"alerts\":%lu}",
                   part[0], part[1], (unsigned long)rejected,
                   (unsigned long)alerts());
  if (n <= 0 || size_t(n) >= capacity) {
    *out = 0;
    return false;
  }
  return true;
}

} // namespace sentinel::cebo
