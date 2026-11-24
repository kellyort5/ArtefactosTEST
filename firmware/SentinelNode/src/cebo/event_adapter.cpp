// Adaptador de eventos: convierte una interacción clasificada en los detalles
// v2 exactos de `cebo.interaction`. No conserva ni recibe datos del cliente más
// allá de la IPv4, los puertos, el tipo, la ruta canónica y la evidencia.
#include "include/cebo.hpp"
#include <cstdio>
#include <cstring>

namespace sentinel::cebo {

const char *suspicion_text(Suspicion s) {
  switch (s) {
  case Suspicion::Medium:
    return "medium";
  case Suspicion::High:
    return "high";
  case Suspicion::Informational:
    break;
  }
  return "informational";
}

// El contrato liga sospecha y severidad: informational→info, medium→warning,
// high→critical.
const char *severity_text(Suspicion s) {
  switch (s) {
  case Suspicion::Medium:
    return "warning";
  case Suspicion::High:
    return "critical";
  case Suspicion::Informational:
    break;
  }
  return "info";
}

bool format_details(const Interaction &i, const Classification &c, char *out,
                    size_t capacity) {
  if (!out || !capacity)
    return false;
  *out = 0;
  // El contrato exige puertos 1..65535 y, para Telnet, puerto 23 sin ruta.
  if (!i.source_port || !i.destination_port)
    return false;
  if (i.protocol == Protocol::Telnet &&
      (i.destination_port != 23 || i.path != Path::None))
    return false;
  if (!c.count || !c.window_ms || !c.evidence_count || c.evidence_count > 2)
    return false;
  char address[16];
  snprintf(address, sizeof(address), "%u.%u.%u.%u", i.ipv4[0], i.ipv4[1],
           i.ipv4[2], i.ipv4[3]);
  const char *path = path_text(i.path);
  char path_json[24];
  if (path)
    snprintf(path_json, sizeof(path_json), "\"%s\"", path);
  else
    snprintf(path_json, sizeof(path_json), "null");
  char evidence[96];
  int e = snprintf(evidence, sizeof(evidence), "\"%s\"", c.evidence[0]);
  if (c.evidence_count == 2 && e > 0 && size_t(e) < sizeof(evidence))
    e += snprintf(evidence + e, sizeof(evidence) - size_t(e), ",\"%s\"",
                  c.evidence[1]);
  if (e <= 0 || size_t(e) >= sizeof(evidence))
    return false;
  int n = snprintf(out, capacity,
                   "{\"source_ipv4\":\"%s\",\"source_port\":%u,"
                   "\"destination_port\":%u,\"protocol\":\"%s\","
                   "\"interaction\":\"%s\",\"path\":%s,\"count\":%u,"
                   "\"window_ms\":%u,\"suspicion\":\"%s\","
                   "\"evidence\":[%s]}",
                   address, unsigned(i.source_port),
                   unsigned(i.destination_port), protocol_text(i.protocol),
                   kind_text(i.kind), path_json, unsigned(c.count),
                   unsigned(c.window_ms), suspicion_text(c.suspicion),
                   evidence);
  if (n <= 0 || size_t(n) >= capacity) {
    *out = 0;
    return false;
  }
  return true;
}

} // namespace sentinel::cebo
