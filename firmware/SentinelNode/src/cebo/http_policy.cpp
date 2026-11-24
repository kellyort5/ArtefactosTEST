#include "include/cebo.hpp"
#include <cstring>

namespace sentinel::cebo {
namespace {
// Páginas y respuestas fijas: nada recibido del cliente se refleja en ellas.
const char PanelPage[] =
    "<!doctype html><html lang=\"es\"><head><meta charset=\"utf-8\">"
    "<title>Panel de administracion</title></head><body>"
    "<h1>Panel de administracion</h1>"
    "<form method=\"post\" action=\"/login\">"
    "<p><label>Usuario <input name=\"user\" autocomplete=\"off\"></label></p>"
    "<p><label>Clave <input name=\"pass\" type=\"password\" "
    "autocomplete=\"off\"></label></p>"
    "<p><button type=\"submit\">Entrar</button></p></form></body></html>";
const char FailedPage[] =
    "<!doctype html><html lang=\"es\"><head><meta charset=\"utf-8\">"
    "<title>Panel de administracion</title></head><body>"
    "<h1>Panel de administracion</h1>"
    "<p>Usuario o clave incorrectos.</p>"
    "<form method=\"post\" action=\"/login\">"
    "<p><label>Usuario <input name=\"user\" autocomplete=\"off\"></label></p>"
    "<p><label>Clave <input name=\"pass\" type=\"password\" "
    "autocomplete=\"off\"></label></p>"
    "<p><button type=\"submit\">Entrar</button></p></form></body></html>";
const char RobotsText[] = "User-agent: *\nDisallow: /admin\n";
const char NotFoundText[] = "Not found\n";
const char TooLargeText[] = "Payload too large\n";
const char NotAllowedText[] = "Method not allowed\n";
const char Html[] = "text/html; charset=utf-8";
const char Plain[] = "text/plain; charset=utf-8";

const Response PanelResponse = {"200 OK", Html, PanelPage, nullptr, false};
const Response FailedResponse = {"200 OK", Html, FailedPage, nullptr, false};
const Response RobotsResponse = {"200 OK", Plain, RobotsText, nullptr, false};
const Response NotFoundResponse = {"404 Not Found", Plain, NotFoundText,
                                   nullptr, false};
const Response TooLargeResponse = {"413 Payload Too Large", Plain,
                                   TooLargeText, nullptr, true};
const Response NotAllowedResponse = {"405 Method Not Allowed", Plain,
                                     NotAllowedText, "GET, HEAD, POST", true};

void record(InteractionBus &bus, const Peer &peer, Kind kind, Path path,
            uint64_t now_ms) {
  Interaction i;
  memcpy(i.ipv4, peer.ipv4, sizeof(i.ipv4));
  i.source_port = peer.port;
  i.destination_port = HttpPort;
  i.protocol = Protocol::Http;
  i.kind = kind;
  i.path = path;
  i.uptime_ms = now_ms;
  bus.push(i);
}

// Escritura volátil: el compilador no puede eliminar el borrado del buffer.
void wipe(void *data, size_t length) {
  volatile uint8_t *p = static_cast<volatile uint8_t *>(data);
  while (length--)
    *p++ = 0;
}

// Consume el cuerpo sin interpretarlo. Devuelve falso ante error o tiempo
// agotado; el contenido nunca sale de este buffer transitorio.
bool discard_body(BodyReader &reader, size_t length) {
  uint8_t sink[128];
  bool ok = true;
  while (length) {
    size_t want = length < sizeof(sink) ? length : sizeof(sink);
    int got = reader.read(sink, want);
    if (got <= 0 || size_t(got) > want) {
      ok = false;
      break;
    }
    length -= size_t(got);
  }
  wipe(sink, sizeof(sink));
  return ok;
}

Response page_for(Path path) {
  switch (path) {
  case Path::Root:
  case Path::Admin:
  case Path::Login:
    return PanelResponse;
  case Path::Robots:
    return RobotsResponse;
  default:
    return NotFoundResponse;
  }
}
} // namespace

Path canonical_path(const char *uri) {
  if (!uri || *uri != '/')
    return Path::Other;
  // La consulta y el fragmento se descartan antes de comparar y no se guardan.
  size_t length = strcspn(uri, "?#");
  struct {
    const char *text;
    Path path;
  } const known[] = {{"/", Path::Root},
                     {"/login", Path::Login},
                     {"/admin", Path::Admin},
                     {"/favicon.ico", Path::Favicon},
                     {"/robots.txt", Path::Robots}};
  for (const auto &k : known)
    if (strlen(k.text) == length && !strncmp(uri, k.text, length))
      return k.path;
  return Path::Other;
}

Response handle_request(Method method, const char *uri, size_t content_length,
                        BodyReader &reader, const Peer &peer, uint64_t now_ms,
                        InteractionBus &bus) {
  Path path = canonical_path(uri);
  if (method == Method::Get || method == Method::Head) {
    record(bus, peer, Kind::Access, path, now_ms);
    return page_for(path);
  }
  if (method == Method::Post && path == Path::Login) {
    // El intento queda registrado antes de leer: un cliente lento o un cuerpo
    // excesivo siguen siendo un intento de autenticación.
    record(bus, peer, Kind::AuthAttempt, Path::Login, now_ms);
    if (content_length > HttpMaxBody)
      return TooLargeResponse;
    if (!discard_body(reader, content_length)) {
      Response r = FailedResponse;
      r.close = true;
      return r;
    }
    return FailedResponse;
  }
  // Otros métodos y POST fuera de /login: se registra el acceso, no se lee el
  // cuerpo y se cierra la conexión.
  record(bus, peer, Kind::Access, path, now_ms);
  return NotAllowedResponse;
}

} // namespace sentinel::cebo
