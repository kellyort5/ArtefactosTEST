// Pruebas de host del panel HTTP ficticio de Cebo. No usan radio, sockets ni
// NVS: comprueban la lógica pura (política, bus, ciclo de vida). La
// integración con esp_http_server y las pruebas en LAN real se registran
// aparte como evidencia física.
#include "../../SentinelNode/src/cebo/include/cebo.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace sentinel;
using namespace sentinel::cebo;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,             \
                   #condition);                                                \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

static const char *Marker = "CEBO-SECRET-MARKER-7f3a91";

static bool contains(const void *data, size_t length, const char *needle) {
  return std::string_view(static_cast<const char *>(data), length)
             .find(needle) != std::string_view::npos;
}
static bool contains(const std::string &text, const char *needle) {
  return text.find(needle) != std::string::npos;
}

struct ScriptedBody : BodyReader {
  std::string data;
  size_t offset = 0, fail_at = SIZE_MAX, calls = 0;
  int read(uint8_t *buffer, size_t capacity) override {
    ++calls;
    if (offset >= fail_at)
      return -1;
    size_t n = data.size() - offset;
    if (n > capacity)
      n = capacity;
    if (offset + n > fail_at)
      n = fail_at - offset;
    memcpy(buffer, data.data() + offset, n);
    offset += n;
    return int(n);
  }
};

static Peer peer() {
  Peer p;
  p.ipv4[0] = 192;
  p.ipv4[1] = 0;
  p.ipv4[2] = 2;
  p.ipv4[3] = 2;
  p.port = 45000;
  return p;
}

static std::vector<Interaction> take(InteractionBus &bus) {
  std::vector<Interaction> out;
  Interaction i;
  while (bus.front(i)) {
    out.push_back(i);
    bus.pop();
  }
  return out;
}

static void paths() {
  struct {
    const char *uri;
    Path path;
  } const table[] = {{"/", Path::Root},
                     {"/login", Path::Login},
                     {"/admin", Path::Admin},
                     {"/favicon.ico", Path::Favicon},
                     {"/robots.txt", Path::Robots},
                     {"/login?user=a&pass=b", Path::Login},
                     {"/?x=1", Path::Root},
                     {"/admin#fragment", Path::Admin},
                     {"/admin/", Path::Other},
                     {"/ADMIN", Path::Other},
                     {"/%6Cogin", Path::Other},
                     {"/login/../admin", Path::Other},
                     {"/cgi-bin/luci", Path::Other},
                     {"", Path::Other},
                     {"login", Path::Other}};
  for (const auto &t : table)
    CHECK(canonical_path(t.uri) == t.path);
  CHECK(canonical_path(nullptr) == Path::Other);
}

static void bus_behaviour() {
  InteractionBus bus;
  Interaction i;
  CHECK(!bus.front(i) && bus.size() == 0);
  for (unsigned n = 0; n < BusCapacity; ++n) {
    i.source_port = uint16_t(1000 + n);
    CHECK(bus.push(i));
  }
  i.source_port = 9;
  CHECK(!bus.push(i) && bus.dropped() == 1 && bus.size() == BusCapacity);
  for (unsigned n = 0; n < BusCapacity; ++n) {
    Interaction out;
    CHECK(bus.front(out) && out.source_port == 1000 + n); // conserva el orden
    bus.pop();
  }
  CHECK(bus.size() == 0);
  // El anillo da la vuelta sin perder el orden.
  for (unsigned round = 0; round < 3; ++round)
    for (unsigned n = 0; n < 10; ++n) {
      i.source_port = uint16_t(n + 1);
      CHECK(bus.push(i));
      Interaction out;
      CHECK(bus.front(out) && out.source_port == n + 1);
      bus.pop();
    }
  CHECK(bus.push(i));
  bus.close();
  CHECK(bus.size() == 0 && !bus.push(i));
  bus.open();
  CHECK(bus.push(i) && bus.size() == 1);
}

static void policy() {
  InteractionBus bus;
  ScriptedBody none;
  for (const char *uri : {"/", "/admin", "/login"}) {
    Response r = handle_request(Method::Get, uri, 0, none, peer(), 10, bus);
    CHECK(!strcmp(r.status, "200 OK") && strstr(r.body, "<form") && !r.close);
    CHECK(strstr(r.body, "action=\"/login\""));
  }
  Response r = handle_request(Method::Get, "/robots.txt", 0, none, peer(), 10, bus);
  CHECK(!strcmp(r.status, "200 OK") && strstr(r.body, "Disallow"));
  r = handle_request(Method::Get, "/favicon.ico", 0, none, peer(), 10, bus);
  CHECK(!strcmp(r.status, "404 Not Found"));
  r = handle_request(Method::Get, "/nope", 0, none, peer(), 10, bus);
  CHECK(!strcmp(r.status, "404 Not Found"));
  r = handle_request(Method::Head, "/", 0, none, peer(), 10, bus);
  CHECK(!strcmp(r.status, "200 OK"));
  auto accesses = take(bus);
  CHECK(accesses.size() == 7);
  const Path expected[] = {Path::Root,   Path::Admin,   Path::Login, Path::Robots,
                           Path::Favicon, Path::Other, Path::Root};
  for (size_t n = 0; n < accesses.size(); ++n) {
    CHECK(accesses[n].kind == Kind::Access && accesses[n].path == expected[n]);
    CHECK(accesses[n].protocol == Protocol::Http &&
          accesses[n].destination_port == 80 &&
          accesses[n].source_port == 45000 && accesses[n].ipv4[3] == 2 &&
          accesses[n].uptime_ms == 10);
  }
  CHECK(none.calls == 0); // los GET nunca leen cuerpo

  // POST /login: intento de autenticación, cuerpo vaciado y respuesta fija.
  ScriptedBody form;
  form.data = "user=admin&pass=hunter2";
  r = handle_request(Method::Post, "/login", form.data.size(), form, peer(), 20, bus);
  CHECK(!strcmp(r.status, "200 OK") && strstr(r.body, "incorrectos") && !r.close);
  CHECK(form.offset == form.data.size());
  auto auth = take(bus);
  CHECK(auth.size() == 1 && auth[0].kind == Kind::AuthAttempt &&
        auth[0].path == Path::Login);

  // Límite exacto del cuerpo y exceso.
  ScriptedBody edge;
  edge.data.assign(HttpMaxBody, 'x');
  r = handle_request(Method::Post, "/login", edge.data.size(), edge, peer(), 21, bus);
  CHECK(!strcmp(r.status, "200 OK") && edge.offset == HttpMaxBody);
  ScriptedBody huge;
  huge.data.assign(HttpMaxBody + 1, 'x');
  r = handle_request(Method::Post, "/login", huge.data.size(), huge, peer(), 22, bus);
  CHECK(!strcmp(r.status, "413 Payload Too Large") && r.close && huge.calls == 0);
  auto attempts = take(bus);
  CHECK(attempts.size() == 2 && attempts[0].kind == Kind::AuthAttempt &&
        attempts[1].kind == Kind::AuthAttempt); // el exceso también es un intento

  // Cliente lento o roto: un solo intento, respuesta fija y cierre.
  ScriptedBody broken;
  broken.data.assign(500, 'x');
  broken.fail_at = 200;
  r = handle_request(Method::Post, "/login", broken.data.size(), broken, peer(), 23, bus);
  CHECK(r.close && strstr(r.body, "incorrectos"));
  CHECK(take(bus).size() == 1);

  // POST fuera de /login y métodos desconocidos: acceso, sin leer cuerpo.
  ScriptedBody ignored;
  ignored.data = "payload";
  r = handle_request(Method::Post, "/admin", ignored.data.size(), ignored, peer(), 24, bus);
  CHECK(!strcmp(r.status, "405 Method Not Allowed") && r.close && r.allow &&
        ignored.calls == 0);
  r = handle_request(Method::Other, "/", 0, none, peer(), 25, bus);
  CHECK(!strcmp(r.status, "405 Method Not Allowed"));
  auto rest = take(bus);
  CHECK(rest.size() == 2 && rest[0].kind == Kind::Access &&
        rest[0].path == Path::Admin && rest[1].path == Path::Root);
}

static void privacy() {
  InteractionBus bus;
  std::string uri = std::string("/login?next=") + Marker + "#" + Marker;
  ScriptedBody body;
  body.data = std::string("user=admin&pass=") + Marker + "&token=" + Marker +
              "&cookie=" + Marker;
  std::string output;
  auto collect = [&](const Response &r) {
    output += r.status;
    output += r.content_type;
    output += r.body;
    if (r.allow)
      output += r.allow;
  };
  Response first = handle_request(Method::Post, uri.c_str(), body.data.size(),
                                  body, peer(), 30, bus);
  collect(first);
  CHECK(body.offset == body.data.size());
  ScriptedBody other;
  other.data = "completely different body";
  Response second = handle_request(Method::Post, "/login", other.data.size(),
                                   other, peer(), 31, bus);
  CHECK(!strcmp(first.body, second.body) && !strcmp(first.status, second.status));
  collect(handle_request(Method::Get,
                         (std::string("/admin/") + Marker + "?q=" + Marker).c_str(),
                         0, other, peer(), 32, bus));
  collect(handle_request(Method::Get, (std::string("/") + Marker).c_str(), 0,
                         other, peer(), 33, bus));
  // La memoria del bus no contiene el marcador: no hay dónde guardarlo.
  CHECK(!contains(&bus, sizeof(bus), Marker));
  CHECK(!contains(output, Marker) && !contains(output, "hunter2") &&
        !contains(output, "admin&pass"));
  for (const auto &i : take(bus)) {
    char details[320];
    CHECK(format_details(i, details, sizeof(details)));
    CHECK(!contains(details, Marker) && !contains(details, "token") &&
          !contains(details, "cookie") && !contains(details, "pass"));
  }
}

static std::string read_file(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}
// Texto del objeto "details" de un evento v2 de una sola línea.
static std::string details_of(const std::string &event) {
  size_t start = event.find("\"details\":");
  CHECK(start != std::string::npos);
  start += 10;
  size_t depth = 0, end = start;
  for (; end < event.size(); ++end) {
    if (event[end] == '{')
      ++depth;
    if (event[end] == '}' && --depth == 0)
      break;
  }
  return event.substr(start, end - start + 1);
}
static std::vector<std::string> keys_of(const std::string &object) {
  std::vector<std::string> keys;
  int depth = 0;
  bool in_string = false;
  for (size_t n = 0; n < object.size(); ++n) {
    char c = object[n];
    if (c == '"') {
      size_t end = object.find('"', n + 1);
      if (depth == 1 && !in_string && object[end + 1] == ':')
        keys.push_back(object.substr(n + 1, end - n - 1));
      n = end;
      continue;
    }
    if (c == '{' || c == '[')
      ++depth;
    if (c == '}' || c == ']')
      --depth;
  }
  return keys;
}

static void details_format(const char *fixtures) {
  Interaction connect;
  connect.ipv4[0] = 192;
  connect.ipv4[2] = 2;
  connect.ipv4[3] = 2;
  connect.source_port = 45000;
  connect.destination_port = 80;
  char text[320];
  CHECK(format_details(connect, text, sizeof(text)));
  CHECK(!strcmp(text,
                "{\"source_ipv4\":\"192.0.2.2\",\"source_port\":45000,"
                "\"destination_port\":80,\"protocol\":\"http\","
                "\"interaction\":\"connect\",\"path\":null,\"count\":1,"
                "\"window_ms\":1000,\"suspicion\":\"informational\","
                "\"evidence\":[\"connection_observed\"]}"));
  Interaction auth = connect;
  auth.kind = Kind::AuthAttempt;
  auth.path = Path::Login;
  CHECK(format_details(auth, text, sizeof(text)));
  CHECK(strstr(text, "\"interaction\":\"auth_attempt\",\"path\":\"/login\""));
  CHECK(strstr(text, "\"evidence\":[\"auth_attempt\"]"));
  Interaction access = connect;
  access.kind = Kind::Access;
  access.path = Path::Other;
  CHECK(format_details(access, text, sizeof(text)));
  CHECK(strstr(text, "\"path\":\"/other\"") &&
        strstr(text, "\"evidence\":[\"canonical_access\"]"));
  Interaction bye = connect;
  bye.kind = Kind::Disconnect;
  CHECK(format_details(bye, text, sizeof(text)) &&
        strstr(text, "\"evidence\":[\"peer_closed\"]"));
  bye.close = Close::IdleTimeout;
  CHECK(format_details(bye, text, sizeof(text)) &&
        strstr(text, "\"evidence\":[\"idle_timeout\"]"));
  // Rechaza datos que el contrato no admite y salidas que no caben.
  Interaction invalid = connect;
  invalid.source_port = 0;
  CHECK(!format_details(invalid, text, sizeof(text)) && !*text);
  invalid = connect;
  invalid.protocol = Protocol::Telnet;
  CHECK(!format_details(invalid, text, sizeof(text))); // Telnet exige puerto 23
  invalid.destination_port = 23;
  CHECK(format_details(invalid, text, sizeof(text)));
  invalid.path = Path::Root;
  CHECK(!format_details(invalid, text, sizeof(text))); // y ruta nula
  char tiny[16];
  CHECK(!format_details(connect, tiny, sizeof(tiny)) && !*tiny);
  CHECK(publishable(access) && publishable(auth) && !publishable(connect) &&
        !publishable(bye));

  if (!fixtures) {
    std::cout << "contract fixtures: SKIPPED (no directory given)\n";
    return;
  }
  // Mismo objeto que el fixture compartido (conexión) y mismas claves y orden
  // que el fixture de autenticación repetida.
  std::string base(fixtures);
  auto benign = read_file(base + "/events/cebo.http-benign.valid.json");
  CHECK(!benign.empty());
  CHECK(details_of(benign) ==
        std::string(
            "{\"source_ipv4\":\"192.0.2.2\",\"source_port\":45000,"
            "\"destination_port\":80,\"protocol\":\"http\","
            "\"interaction\":\"connect\",\"path\":null,\"count\":1,"
            "\"window_ms\":1000,\"suspicion\":\"informational\","
            "\"evidence\":[\"connection_observed\"]}"));
  CHECK(format_details(connect, text, sizeof(text)) &&
        details_of(benign) == text);
  auto repeated = read_file(base + "/events/cebo.http-repeated-auth.valid.json");
  CHECK(!repeated.empty());
  CHECK(format_details(auth, text, sizeof(text)));
  CHECK(keys_of(details_of(repeated)) == keys_of(text));
  CHECK(keys_of(text).size() == 10);
  std::cout << "Cebo details vs shared v2 fixtures: PASS\n";
}

// ---- Dobles para el ciclo de vida -----------------------------------------
struct FakeResources : ModuleResources {
  bool set_channel(uint8_t) override { return false; }
  bool scan_passive() override { return false; }
  size_t scan_results(ScanNetwork *, size_t) override { return 0; }
  bool capture(bool) override { return false; }
  bool configure_upstream(const UpstreamConfig &) override { return false; }
  bool configure_ap(const ApConfig &) override { return false; }
  bool napt(bool) override { return false; }
  esp_netif_obj *sta_netif() const override { return nullptr; }
  esp_netif_obj *ap_netif() const override { return nullptr; }
  bool poll(RadioEvent &) override { return false; }
  uint64_t dropped() const override { return 0; }
};
struct Emitted {
  std::string name, severity, details;
  bool simulated;
};
struct FakeContext : ModuleContext {
  FakeResources resources_;
  uint64_t now = 1000;
  bool accept = true;
  std::vector<Emitted> events;
  uint64_t now_ms() const override { return now; }
  ModuleResources &resources() override { return resources_; }
  bool emit_event(const char *name, const char *severity, const char *details,
                  bool simulated) override {
    if (!accept)
      return false;
    events.push_back({name, severity, details, simulated});
    return true;
  }
  bool publish_metrics(uint32_t, const char *) override { return false; }
};
struct FakePlatform : HttpPlatform {
  bool lan = false, start_ok = true, running = false, stuck = false;
  unsigned starts = 0, stops = 0, ticks = 0, connections = 0;
  InteractionBus *bus = nullptr;
  bool lan_ready() override { return lan; }
  bool start(InteractionBus &b, const HttpSettings &settings) override {
    ++starts;
    if (!start_ok)
      return false;
    CHECK(!running); // el módulo nunca arranca un listener ya abierto
    CHECK(settings.idle_timeout_ms == HttpIdleTimeoutMs);
    running = true;
    bus = &b;
    return true;
  }
  void request_stop() override {
    ++stops;
    if (!stuck)
      running = false;
  }
  bool stopped() const override { return !running; }
  unsigned active_connections() const override { return connections; }
  void tick() override { ++ticks; }
};
static std::string state_of(const Cebo &cebo) {
  ModuleSnapshot s;
  cebo.snapshot(s);
  return s.state;
}
static Interaction make(Kind kind, Path path = Path::None) {
  Interaction i;
  i.ipv4[0] = 192;
  i.ipv4[2] = 2;
  i.ipv4[3] = 2;
  i.source_port = 45000;
  i.destination_port = 80;
  i.kind = kind;
  i.path = path;
  return i;
}
static std::string state_text(const char *status, unsigned connections) {
  char text[400];
  snprintf(text, sizeof(text),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":false,\"status\":\"unavailable\"},"
           "\"active_connections\":%u,\"max_connections\":4,"
           "\"idle_timeout_ms\":5000}",
           status, connections);
  return text;
}

static void lifecycle() {
  FakePlatform http;
  FakeContext ctx;
  Cebo cebo(http);
  CHECK(cebo.init(ctx) && cebo.stopped());
  CHECK(state_of(cebo) == state_text("stopped", 0));
  ModuleSnapshot snap;
  cebo.snapshot(snap);
  CHECK(snap.capabilities.honeypot_http && !snap.capabilities.honeypot_telnet &&
        !snap.capabilities.capture && !snap.target_selected);
  // Antes de start() solo se rechazan órdenes y no hay trabajo.
  ModuleReply reply;
  cebo.handle({"cebo.start", nullptr, "c1"}, reply);
  CHECK(reply.error && !strcmp(reply.error, "invalid_state"));
  cebo.tick(1000);
  CHECK(http.starts == 0 && http.ticks == 0);

  // Esperar la LAN: no se abre ningún listener sin red.
  CHECK(cebo.start() && http.starts == 0);
  CHECK(state_of(cebo) == state_text("starting", 0));
  cebo.tick(1001);
  CHECK(http.starts == 0 && cebo.http_status() == Service::Starting);
  http.lan = true;
  cebo.tick(1002);
  CHECK(http.starts == 1 && cebo.http_status() == Service::Running);
  http.connections = 2;
  CHECK(state_of(cebo) == state_text("running", 2));
  http.connections = 9; // el contrato limita active_connections a 6
  CHECK(state_of(cebo).find("\"active_connections\":6") != std::string::npos);
  http.connections = 0;

  // Conexión y cierre circulan por el bus; solo acceso y autenticación
  // producen eventos v2.
  CHECK(http.bus);
  CHECK(http.bus->push(make(Kind::Connect)));
  CHECK(http.bus->push(make(Kind::Access, Path::Root)));
  CHECK(http.bus->push(make(Kind::AuthAttempt, Path::Login)));
  CHECK(http.bus->push(make(Kind::Disconnect)));
  cebo.tick(1003);
  CHECK(ctx.events.size() == 2 && http.bus->size() == 0);
  CHECK(ctx.events[0].name == "cebo.interaction" &&
        ctx.events[0].severity == "info" && !ctx.events[0].simulated);
  CHECK(contains(ctx.events[0].details, "\"interaction\":\"access\",\"path\":\"/\""));
  CHECK(contains(ctx.events[1].details, "\"interaction\":\"auth_attempt\",\"path\":\"/login\""));
  const Counters &c = cebo.counters();
  CHECK(c.published == 2 && c.publish_failed == 0 &&
        c.interactions[unsigned(Kind::Connect)] == 1 &&
        c.interactions[unsigned(Kind::Access)] == 1 &&
        c.interactions[unsigned(Kind::AuthAttempt)] == 1 &&
        c.interactions[unsigned(Kind::Disconnect)] == 1);

  // Un máximo de EventsPerTick por llamada; el resto sigue en orden.
  ctx.events.clear();
  for (unsigned n = 0; n < 10; ++n) {
    Interaction i = make(Kind::Access, Path::Root);
    i.source_port = uint16_t(2000 + n);
    CHECK(http.bus->push(i));
  }
  cebo.tick(1004);
  CHECK(ctx.events.size() == EventsPerTick && http.bus->size() == 6);
  cebo.tick(1005);
  cebo.tick(1006);
  CHECK(ctx.events.size() == 10 && http.bus->size() == 0);
  CHECK(contains(ctx.events[0].details, "\"source_port\":2000") &&
        contains(ctx.events[9].details, "\"source_port\":2009"));

  // Salida llena: se conserva y reintenta en orden; si no se acepta durante
  // EventRetryMs se descarta y se contabiliza.
  ctx.events.clear();
  ctx.accept = false;
  Interaction held = make(Kind::AuthAttempt, Path::Login);
  held.source_port = 3000;
  CHECK(http.bus->push(held));
  cebo.tick(2000);
  cebo.tick(2000 + EventRetryMs - 1);
  CHECK(ctx.events.empty() && http.bus->size() == 1);
  ctx.accept = true;
  cebo.tick(2000 + EventRetryMs - 1);
  CHECK(ctx.events.size() == 1 && contains(ctx.events[0].details, "\"source_port\":3000"));
  unsigned failed = cebo.counters().publish_failed;
  ctx.accept = false;
  CHECK(http.bus->push(held));
  cebo.tick(9000);
  cebo.tick(9000 + EventRetryMs);
  CHECK(http.bus->size() == 0 && cebo.counters().publish_failed == failed + 1);
  ctx.accept = true;

  // Pérdida de LAN: se cierra y se reabre al volver.
  http.lan = false;
  cebo.tick(10000);
  CHECK(http.stops >= 1 && !http.running && cebo.http_status() == Service::Starting);
  CHECK(state_of(cebo) == state_text("starting", 0));
  cebo.tick(10001);
  CHECK(http.starts == 1);
  http.lan = true;
  cebo.tick(10002);
  CHECK(http.starts == 2 && cebo.http_status() == Service::Running);

  // cebo.stop detiene el servicio y se queda en el modo; cebo.start reanuda.
  unsigned stops = http.stops;
  reply = ModuleReply();
  cebo.handle({"cebo.stop", nullptr, "c2"}, reply);
  CHECK(!reply.error && !*reply.result && !reply.job);
  CHECK(http.stops == stops + 1 && !http.running);
  cebo.tick(10003);
  CHECK(cebo.http_status() == Service::Stopped &&
        state_of(cebo) == state_text("stopped", 0));
  cebo.tick(10004);
  CHECK(http.starts == 2); // detenido a petición: no se reabre solo
  reply = ModuleReply();
  cebo.handle({"cebo.start", nullptr, "c3"}, reply);
  CHECK(!reply.error);
  cebo.tick(10005);
  CHECK(http.starts == 3 && cebo.http_status() == Service::Running);
  for (const char *operation : {"cebo.configure", "cebo.stats", "escudo.scan"}) {
    reply = ModuleReply();
    cebo.handle({operation, nullptr, "c4"}, reply);
    CHECK(reply.error && !strcmp(reply.error, "not_implemented"));
  }

  // Parada: stopped() solo es verdadero cuando el listener se liberó; no
  // quedan interacciones ni eventos tras cerrar.
  size_t events = ctx.events.size();
  http.stuck = true;
  cebo.request_stop();
  CHECK(!cebo.stopped() && http.bus->size() == 0);
  CHECK(!http.bus->push(make(Kind::Access, Path::Root)));
  stops = http.stops;
  cebo.tick(11000);
  CHECK(http.stops == stops + 1 && !cebo.stopped()); // reintenta el cierre
  http.stuck = false;
  cebo.tick(11001);
  CHECK(cebo.stopped() && cebo.http_status() == Service::Stopped);
  CHECK(state_of(cebo) == state_text("stopped", 0));
  cebo.tick(11002);
  CHECK(http.starts == 3 && ctx.events.size() == events);
  reply = ModuleReply();
  cebo.handle({"cebo.start", nullptr, "c5"}, reply);
  CHECK(reply.error && !strcmp(reply.error, "invalid_state"));

  // Una parada que se completa de inmediato no deja estado residual.
  CHECK(cebo.start());
  http.lan = true;
  cebo.tick(12000);
  CHECK(http.starts == 4 && http.running);
  cebo.request_stop();
  CHECK(cebo.stopped() && state_of(cebo) == state_text("stopped", 0));
  cebo.tick(12001);
  CHECK(http.starts == 4);

  // Un arranque nuevo reinicia contadores y bus.
  CHECK(cebo.start());
  CHECK(cebo.counters().published == 0 && cebo.bus().size() == 0);
}

static void start_failure() {
  FakePlatform http;
  FakeContext ctx;
  Cebo cebo(http);
  CHECK(cebo.init(ctx) && cebo.start());
  http.lan = true;
  http.start_ok = false;
  cebo.tick(1000);
  CHECK(http.starts == 1 && cebo.http_status() == Service::Error);
  CHECK(state_of(cebo) == state_text("error", 0));
  cebo.tick(1000 + HttpRetryMs - 1);
  CHECK(http.starts == 1);
  http.start_ok = true;
  cebo.tick(1000 + HttpRetryMs);
  CHECK(http.starts == 2 && cebo.http_status() == Service::Running);
}

// Muestras reales del código para validarlas con el decodificador compartido
// (check_contracts.mjs): una línea por muestra, etiqueta y JSON.
static void dump() {
  char text[320];
  Interaction base = make(Kind::Connect);
  const struct {
    Kind kind;
    Path path;
    Close close;
  } samples[] = {{Kind::Connect, Path::None, Close::None},
                 {Kind::Access, Path::Root, Close::None},
                 {Kind::Access, Path::Admin, Close::None},
                 {Kind::Access, Path::Favicon, Close::None},
                 {Kind::Access, Path::Robots, Close::None},
                 {Kind::Access, Path::Other, Close::None},
                 {Kind::AuthAttempt, Path::Login, Close::None},
                 {Kind::Disconnect, Path::None, Close::Peer},
                 {Kind::Disconnect, Path::None, Close::IdleTimeout}};
  for (const auto &s : samples) {
    Interaction i = base;
    i.kind = s.kind;
    i.path = s.path;
    i.close = s.close;
    CHECK(format_details(i, text, sizeof(text)));
    std::cout << "details " << text << '\n';
  }
  FakePlatform http;
  FakeContext ctx;
  Cebo cebo(http);
  CHECK(cebo.init(ctx));
  std::cout << "module " << state_of(cebo) << '\n'; // stopped
  CHECK(cebo.start());
  std::cout << "module " << state_of(cebo) << '\n'; // starting
  http.lan = true;
  cebo.tick(1000);
  http.connections = 3;
  std::cout << "module " << state_of(cebo) << '\n'; // running
  http.connections = 99;
  std::cout << "module " << state_of(cebo) << '\n'; // limitado a 6
  FakePlatform failing;
  failing.start_ok = false;
  failing.lan = true;
  Cebo broken(failing);
  CHECK(broken.init(ctx) && broken.start());
  broken.tick(1000);
  std::cout << "module " << state_of(broken) << '\n'; // error
}

int main(int argc, char **argv) {
  if (argc > 1 && !strcmp(argv[1], "dump")) {
    dump();
    return 0;
  }
  paths();
  std::cout << "Cebo canonical paths: PASS\n";
  bus_behaviour();
  std::cout << "Cebo interaction bus (bounded, ordered, closable): PASS\n";
  policy();
  std::cout << "Cebo HTTP policy (routes, limits, fixed responses): PASS\n";
  privacy();
  std::cout << "Cebo privacy (secret marker absent from every output): PASS\n";
  details_format(argc > 1 ? argv[1] : nullptr);
  lifecycle();
  start_failure();
  std::cout << "Cebo module lifecycle (LAN wait, drain, stop, restart): PASS\n";
  return 0;
}
