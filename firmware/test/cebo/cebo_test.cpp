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
struct FakeListener : ServiceListener {
  bool start_ok = true, running = false, stuck = false;
  unsigned starts = 0, stops = 0, ticks = 0, connections = 0;
  InteractionBus *bus = nullptr;
  bool start(InteractionBus &b, const ServiceSettings &settings) override {
    ++starts;
    if (!start_ok)
      return false;
    CHECK(!running); // el módulo nunca arranca un listener ya abierto
    CHECK(settings.idle_timeout_ms == IdleTimeoutMs);
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
struct FakeCebo : CeboPlatform {
  bool lan = false;
  FakeListener h, t;
  bool lan_ready() override { return lan; }
  ServiceListener &http() override { return h; }
  ServiceListener &telnet() override { return t; }
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
static Interaction make_telnet(Kind kind, Close close = Close::None) {
  Interaction i = make(kind);
  i.protocol = Protocol::Telnet;
  i.destination_port = 23;
  i.close = close;
  return i;
}
static std::string state_text(const char *http, const char *telnet,
                              unsigned connections) {
  char text[420];
  snprintf(text, sizeof(text),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":true,\"status\":\"%s\"},"
           "\"active_connections\":%u,\"max_connections\":6,"
           "\"idle_timeout_ms\":5000}",
           http, telnet, connections);
  return text;
}

static void lifecycle() {
  FakeCebo p;
  FakeContext ctx;
  Cebo cebo(p);
  CHECK(cebo.init(ctx) && cebo.stopped());
  CHECK(state_of(cebo) == state_text("stopped", "stopped", 0));
  ModuleSnapshot snap;
  cebo.snapshot(snap);
  CHECK(snap.capabilities.honeypot_http && snap.capabilities.honeypot_telnet &&
        !snap.capabilities.capture && !snap.target_selected);
  // Antes de start() solo se rechazan órdenes y no hay trabajo.
  ModuleReply reply;
  cebo.handle({"cebo.start", nullptr, "c1"}, reply);
  CHECK(reply.error && !strcmp(reply.error, "invalid_state"));
  cebo.tick(1000);
  CHECK(p.h.starts == 0 && p.t.starts == 0 && p.h.ticks == 0);

  // Esperar la LAN: ningún listener se abre sin red.
  CHECK(cebo.start() && p.h.starts == 0 && p.t.starts == 0);
  CHECK(state_of(cebo) == state_text("starting", "starting", 0));
  cebo.tick(1001);
  CHECK(p.h.starts == 0 && p.t.starts == 0 &&
        cebo.http_status() == Service::Starting &&
        cebo.telnet_status() == Service::Starting);
  p.lan = true;
  cebo.tick(1002);
  CHECK(p.h.starts == 1 && p.t.starts == 1 &&
        cebo.http_status() == Service::Running &&
        cebo.telnet_status() == Service::Running);
  p.h.connections = 2;
  p.t.connections = 1;
  CHECK(state_of(cebo) == state_text("running", "running", 3));
  p.h.connections = 9; // el contrato limita active_connections a 6
  p.t.connections = 9;
  CHECK(state_of(cebo).find("\"active_connections\":6") != std::string::npos);
  p.h.connections = p.t.connections = 0;

  // Los dos servicios comparten un único bus. Conexión y cierre circulan por
  // él; acceso, entrada y autenticación producen eventos v2.
  CHECK(p.h.bus && p.h.bus == p.t.bus && p.h.bus == &cebo.bus());
  InteractionBus &bus = *p.h.bus;
  CHECK(bus.push(make(Kind::Connect)));
  CHECK(bus.push(make(Kind::Access, Path::Root)));
  CHECK(bus.push(make_telnet(Kind::Connect)));
  CHECK(bus.push(make_telnet(Kind::Input)));
  CHECK(bus.push(make_telnet(Kind::AuthAttempt)));
  CHECK(bus.push(make(Kind::AuthAttempt, Path::Login)));
  CHECK(bus.push(make(Kind::Disconnect)));
  CHECK(bus.push(make_telnet(Kind::Disconnect, Close::Limit)));
  cebo.tick(1003);
  cebo.tick(1004);
  CHECK(ctx.events.size() == 4 && bus.size() == 0);
  for (const auto &e : ctx.events)
    CHECK(e.name == "cebo.interaction" && e.severity == "info" && !e.simulated);
  CHECK(contains(ctx.events[0].details,
                 "\"protocol\":\"http\",\"interaction\":\"access\",\"path\":\"/\""));
  CHECK(contains(ctx.events[1].details,
                 "\"destination_port\":23,\"protocol\":\"telnet\","
                 "\"interaction\":\"input\",\"path\":null"));
  CHECK(contains(ctx.events[2].details,
                 "\"protocol\":\"telnet\",\"interaction\":\"auth_attempt\","
                 "\"path\":null"));
  CHECK(contains(ctx.events[3].details,
                 "\"protocol\":\"http\",\"interaction\":\"auth_attempt\","
                 "\"path\":\"/login\""));
  const Counters &c = cebo.counters();
  CHECK(c.published == 4 && c.publish_failed == 0 &&
        c.interactions[unsigned(Kind::Connect)] == 2 &&
        c.interactions[unsigned(Kind::Access)] == 1 &&
        c.interactions[unsigned(Kind::Input)] == 1 &&
        c.interactions[unsigned(Kind::AuthAttempt)] == 2 &&
        c.interactions[unsigned(Kind::Disconnect)] == 2);

  // Un máximo de EventsPerTick por llamada; el resto sigue en orden.
  ctx.events.clear();
  for (unsigned n = 0; n < 10; ++n) {
    Interaction i = make(Kind::Access, Path::Root);
    i.source_port = uint16_t(2000 + n);
    CHECK(bus.push(i));
  }
  cebo.tick(1005);
  CHECK(ctx.events.size() == EventsPerTick && bus.size() == 6);
  cebo.tick(1006);
  cebo.tick(1007);
  CHECK(ctx.events.size() == 10 && bus.size() == 0);
  CHECK(contains(ctx.events[0].details, "\"source_port\":2000") &&
        contains(ctx.events[9].details, "\"source_port\":2009"));

  // Salida llena: se conserva y reintenta en orden; si no se acepta durante
  // EventRetryMs se descarta y se contabiliza.
  ctx.events.clear();
  ctx.accept = false;
  Interaction held = make(Kind::AuthAttempt, Path::Login);
  held.source_port = 3000;
  CHECK(bus.push(held));
  cebo.tick(2000);
  cebo.tick(2000 + EventRetryMs - 1);
  CHECK(ctx.events.empty() && bus.size() == 1);
  ctx.accept = true;
  cebo.tick(2000 + EventRetryMs - 1);
  CHECK(ctx.events.size() == 1 &&
        contains(ctx.events[0].details, "\"source_port\":3000"));
  unsigned failed = cebo.counters().publish_failed;
  ctx.accept = false;
  CHECK(bus.push(held));
  cebo.tick(9000);
  cebo.tick(9000 + EventRetryMs);
  CHECK(bus.size() == 0 && cebo.counters().publish_failed == failed + 1);
  ctx.accept = true;

  // Pérdida de LAN: los dos servicios se cierran y se reabren al volver.
  p.lan = false;
  cebo.tick(10000);
  CHECK(p.h.stops >= 1 && p.t.stops >= 1 && !p.h.running && !p.t.running);
  CHECK(cebo.http_status() == Service::Starting &&
        cebo.telnet_status() == Service::Starting);
  CHECK(state_of(cebo) == state_text("starting", "starting", 0));
  cebo.tick(10001);
  CHECK(p.h.starts == 1 && p.t.starts == 1);
  p.lan = true;
  cebo.tick(10002);
  CHECK(p.h.starts == 2 && p.t.starts == 2 &&
        cebo.http_status() == Service::Running &&
        cebo.telnet_status() == Service::Running);

  // cebo.stop detiene los dos servicios y se queda en el modo; cebo.start los
  // reanuda y reabre el bus.
  unsigned stops_h = p.h.stops, stops_t = p.t.stops;
  reply = ModuleReply();
  cebo.handle({"cebo.stop", nullptr, "c2"}, reply);
  CHECK(!reply.error && !*reply.result && !reply.job);
  CHECK(p.h.stops == stops_h + 1 && p.t.stops == stops_t + 1);
  CHECK(!p.h.running && !p.t.running && !bus.push(make(Kind::Access, Path::Root)));
  cebo.tick(10003);
  CHECK(cebo.http_status() == Service::Stopped &&
        cebo.telnet_status() == Service::Stopped &&
        state_of(cebo) == state_text("stopped", "stopped", 0));
  cebo.tick(10004);
  CHECK(p.h.starts == 2 && p.t.starts == 2); // detenido a petición
  reply = ModuleReply();
  cebo.handle({"cebo.start", nullptr, "c3"}, reply);
  CHECK(!reply.error);
  CHECK(bus.push(make(Kind::Access, Path::Root)));
  cebo.tick(10005);
  CHECK(p.h.starts == 3 && p.t.starts == 3 &&
        cebo.http_status() == Service::Running &&
        cebo.telnet_status() == Service::Running);
  reply = ModuleReply();
  cebo.handle({"cebo.start", nullptr, "c3b"}, reply); // ya activo: sin efecto
  CHECK(!reply.error && p.h.starts == 3);
  for (const char *operation : {"cebo.configure", "cebo.stats", "escudo.scan"}) {
    reply = ModuleReply();
    cebo.handle({operation, nullptr, "c4"}, reply);
    CHECK(reply.error && !strcmp(reply.error, "not_implemented"));
  }

  // Parada: stopped() solo es verdadero cuando los DOS listeners se liberaron;
  // no quedan interacciones ni eventos tras cerrar.
  size_t events = ctx.events.size();
  p.h.stuck = p.t.stuck = true;
  cebo.request_stop();
  CHECK(!cebo.stopped() && bus.size() == 0);
  CHECK(!bus.push(make(Kind::Access, Path::Root)));
  stops_h = p.h.stops;
  stops_t = p.t.stops;
  cebo.tick(11000);
  CHECK(p.h.stops == stops_h + 1 && p.t.stops == stops_t + 1 && !cebo.stopped());
  p.t.stuck = false; // solo Telnet libera: el módulo sigue sin estar detenido
  cebo.tick(11001);
  CHECK(!cebo.stopped() && p.h.stops == stops_h + 2);
  p.h.stuck = false;
  cebo.tick(11002);
  CHECK(cebo.stopped() && cebo.http_status() == Service::Stopped &&
        cebo.telnet_status() == Service::Stopped);
  CHECK(state_of(cebo) == state_text("stopped", "stopped", 0));
  cebo.tick(11003);
  CHECK(p.h.starts == 3 && p.t.starts == 3 && ctx.events.size() == events);
  reply = ModuleReply();
  cebo.handle({"cebo.start", nullptr, "c5"}, reply);
  CHECK(reply.error && !strcmp(reply.error, "invalid_state"));

  // Una parada que se completa de inmediato no deja estado residual.
  CHECK(cebo.start());
  cebo.tick(12000);
  CHECK(p.h.starts == 4 && p.t.starts == 4 && p.h.running && p.t.running);
  cebo.request_stop();
  CHECK(cebo.stopped() && state_of(cebo) == state_text("stopped", "stopped", 0));
  cebo.tick(12001);
  CHECK(p.h.starts == 4 && p.t.starts == 4);

  // Un arranque nuevo reinicia contadores y bus.
  CHECK(cebo.start());
  CHECK(cebo.counters().published == 0 && cebo.bus().size() == 0);
}

static void independent_failures() {
  FakeCebo p;
  FakeContext ctx;
  Cebo cebo(p);
  CHECK(cebo.init(ctx) && cebo.start());
  p.lan = true;
  p.t.start_ok = false;
  cebo.tick(1000);
  // Un fallo de Telnet no impide HTTP, y viceversa en el siguiente caso.
  CHECK(p.h.starts == 1 && p.t.starts == 1);
  CHECK(cebo.http_status() == Service::Running &&
        cebo.telnet_status() == Service::Error);
  CHECK(state_of(cebo) == state_text("running", "error", 0));
  cebo.tick(1000 + ServiceRetryMs - 1);
  CHECK(p.t.starts == 1 && p.h.starts == 1);
  p.t.start_ok = true;
  cebo.tick(1000 + ServiceRetryMs);
  CHECK(p.t.starts == 2 && cebo.telnet_status() == Service::Running &&
        p.h.starts == 1);

  FakeCebo q;
  Cebo other(q);
  CHECK(other.init(ctx) && other.start());
  q.lan = true;
  q.h.start_ok = false;
  other.tick(2000);
  CHECK(other.http_status() == Service::Error &&
        other.telnet_status() == Service::Running);
  CHECK(state_of(other) == state_text("error", "running", 0));
  q.h.start_ok = true;
  other.tick(2000 + ServiceRetryMs);
  CHECK(other.http_status() == Service::Running);
}

// ---- Sesión Telnet ----------------------------------------------------------
static Peer telnet_peer() {
  Peer p;
  p.ipv4[0] = 10;
  p.ipv4[3] = 9;
  p.port = 51000;
  return p;
}
static std::string out_of(TelnetSession &s) {
  std::string out(reinterpret_cast<const char *>(s.output()), s.output_length());
  s.clear_output();
  return out;
}
static std::string raw(std::initializer_list<int> bytes) {
  std::string out;
  for (int b : bytes)
    out.push_back(char(b));
  return out;
}
// Entrega los datos de uno en uno: el peor caso de fragmentación.
static bool feed_each(TelnetSession &s, const std::string &data, uint64_t now = 200) {
  bool keep = true;
  for (char c : data) {
    uint8_t b = uint8_t(c);
    keep = s.feed(&b, 1, now);
  }
  return keep;
}
static bool feed_all(TelnetSession &s, const std::string &data, uint64_t now = 200) {
  return s.feed(reinterpret_cast<const uint8_t *>(data.data()), data.size(), now);
}
// Textos fijos esperados.
static const std::string Banner = "\r\nRouter management console\r\n\r\nlogin: ";
static const std::string PasswordPrompt = raw({255, 251, 1}) + "Password: ";
static const std::string LoginFailed =
    raw({255, 252, 1}) + "\r\nLogin incorrect\r\n\r\nlogin: ";
static const std::string TooMany =
    raw({255, 252, 1}) + "\r\nLogin incorrect\r\n\r\nToo many failed attempts\r\n";

static void telnet_flow() {
  InteractionBus bus;
  TelnetSession s;
  s.open(telnet_peer(), 100, bus);
  CHECK(out_of(s) == Banner && !s.closing());
  auto opened = take(bus);
  CHECK(opened.size() == 1 && opened[0].kind == Kind::Connect &&
        opened[0].protocol == Protocol::Telnet &&
        opened[0].destination_port == 23 && opened[0].path == Path::None &&
        opened[0].source_port == 51000 && opened[0].ipv4[0] == 10 &&
        opened[0].ipv4[3] == 9 && opened[0].uptime_ms == 100);

  // Usuario fragmentado, con CR y LF en entregas distintas.
  CHECK(feed_each(s, "root") && out_of(s).empty());
  CHECK(feed_all(s, "\r") && out_of(s) == PasswordPrompt);
  CHECK(feed_all(s, "\n") && out_of(s).empty()); // el LF de CR LF no es otra línea
  auto input = take(bus);
  CHECK(input.size() == 1 && input[0].kind == Kind::Input &&
        input[0].path == Path::None && input[0].protocol == Protocol::Telnet);

  // Contraseña: sin eco, un solo intento, de vuelta al login.
  CHECK(feed_each(s, "hunter2") && out_of(s).empty());
  CHECK(feed_all(s, std::string("\r\0", 2)) && out_of(s) == LoginFailed);
  auto attempt = take(bus);
  CHECK(attempt.size() == 1 && attempt[0].kind == Kind::AuthAttempt);
  CHECK(!s.closing() && !s.finished());

  // Tercer intento: respuesta de cierre y cierre por límite.
  CHECK(feed_all(s, "a\r\nb\r\n") && out_of(s) == PasswordPrompt + LoginFailed);
  CHECK(take(bus).size() == 2);
  CHECK(!feed_all(s, "c\r\nd\r\n") && s.closing() && s.close_reason() == Close::Limit);
  CHECK(out_of(s) == PasswordPrompt + TooMany);
  auto last = take(bus);
  CHECK(last.size() == 2 && last[0].kind == Kind::Input &&
        last[1].kind == Kind::AuthAttempt);
  // Cerrada la sesión, no procesa nada más ni genera interacciones.
  CHECK(!feed_all(s, "e\r\n") && take(bus).empty() && out_of(s).empty());
  s.finish(Close::Limit, 300);
  auto closed = take(bus);
  CHECK(closed.size() == 1 && closed[0].kind == Kind::Disconnect &&
        closed[0].close == Close::Limit && closed[0].uptime_ms == 300);
  s.finish(Close::Peer, 400); // una sola desconexión por sesión
  CHECK(take(bus).empty() && s.finished());
}

static void telnet_pipelined() {
  InteractionBus bus;
  TelnetSession s;
  s.open(telnet_peer(), 100, bus);
  s.clear_output();
  // Tres intentos completos en una sola entrega.
  CHECK(!feed_all(s, "u\r\np\r\nu\r\np\r\nu\r\np\r\n"));
  CHECK(out_of(s) == PasswordPrompt + LoginFailed + PasswordPrompt +
                         LoginFailed + PasswordPrompt + TooMany);
  auto all = take(bus);
  CHECK(all.size() == 7); // conexión, tres entradas y tres autenticaciones
  unsigned inputs = 0, auths = 0;
  for (const auto &i : all) {
    inputs += i.kind == Kind::Input;
    auths += i.kind == Kind::AuthAttempt;
  }
  CHECK(inputs == 3 && auths == 3);
}

static void telnet_line_endings() {
  struct {
    std::string text;
    unsigned lines;
  } const cases[] = {{"abc\r\n", 1},  {std::string("abc\r\0", 5), 1},
                     {"abc\n", 1},    {"abc\r", 1},
                     {"abc\r\r", 2},  {"\r\n\r\n", 2},
                     {"abc\r\nxyz", 1}, {"a\nb\n", 2}};
  for (const auto &c : cases) {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    take(bus);
    CHECK(feed_each(s, c.text));
    CHECK(take(bus).size() == c.lines);
  }
}

static void telnet_iac() {
  InteractionBus bus;
  TelnetSession s;
  s.open(telnet_peer(), 100, bus);
  s.clear_output();
  take(bus);
  // Se rechazan las opciones y no se responde a WONT/DONT.
  CHECK(feed_all(s, raw({255, 253, 24})) && out_of(s) == raw({255, 252, 24}));
  CHECK(feed_all(s, raw({255, 251, 31})) && out_of(s) == raw({255, 254, 31}));
  CHECK(feed_all(s, raw({255, 252, 5, 255, 254, 5})) && out_of(s).empty());
  // DO/DONT ECHO responden a nuestra oferta: sin acción.
  CHECK(feed_all(s, raw({255, 253, 1, 255, 254, 1})) && out_of(s).empty());
  // IAC fragmentado entre entregas.
  CHECK(feed_each(s, raw({255, 253, 39})) && out_of(s) == raw({255, 252, 39}));
  // Subnegociación completa y comandos sueltos se ignoran.
  CHECK(feed_all(s, raw({255, 250, 24, 1, 255, 240, 255, 241, 255, 246})) &&
        out_of(s).empty());
  CHECK(take(bus).empty());
  // IAC IAC es un dato más; la línea se cierra con normalidad.
  CHECK(feed_all(s, raw({255, 255, 255, 255, '\r'})) && out_of(s) == PasswordPrompt);
  auto line = take(bus);
  CHECK(line.size() == 1 && line[0].kind == Kind::Input);
  // Subnegociación sin terminar: no se traga datos de forma indefinida.
  InteractionBus bus2;
  TelnetSession t;
  t.open(telnet_peer(), 100, bus2);
  t.clear_output();
  take(bus2);
  std::string sub = raw({255, 250}) + std::string(70, 'x') + "ab\r";
  CHECK(feed_all(t, sub) && take(bus2).size() == 1);
  // El número de respuestas por sesión está limitado.
  InteractionBus bus3;
  TelnetSession u;
  u.open(telnet_peer(), 100, bus3);
  u.clear_output();
  for (int option = 10; option < 40; ++option)
    CHECK(feed_all(u, raw({255, 253, option})));
  CHECK(out_of(u).size() == 16 * 3 && !u.closing());
}

static void telnet_limits() {
  // Exactamente la línea máxima es válida; un byte más se rechaza.
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    s.clear_output();
    take(bus);
    CHECK(feed_all(s, std::string(TelnetMaxLine, 'a')) && out_of(s).empty());
    CHECK(feed_all(s, "\r") && out_of(s) == PasswordPrompt);
    CHECK(take(bus).size() == 1);
  }
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    s.clear_output();
    take(bus);
    CHECK(!feed_each(s, std::string(TelnetMaxLine + 1, 'a')));
    CHECK(s.closing() && s.close_reason() == Close::Limit);
    CHECK(out_of(s) == "\r\nInput too long\r\n");
    auto overlong = take(bus);
    CHECK(overlong.size() == 1 && overlong[0].kind == Kind::Input);
    CHECK(!feed_all(s, "x\r\n") && take(bus).empty());
  }
  // En la etapa de contraseña la línea excesiva cuenta como autenticación.
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    CHECK(feed_all(s, "root\r\n"));
    s.clear_output();
    take(bus);
    CHECK(!feed_all(s, std::string(TelnetMaxLine + 1, 'a')));
    auto overlong = take(bus);
    CHECK(overlong.size() == 1 && overlong[0].kind == Kind::AuthAttempt);
  }
  // El borrado no permite escapar del límite ni bajar de cero.
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    s.clear_output();
    CHECK(feed_all(s, std::string(TelnetMaxLine, 'a') + "\x08" + "b") &&
          !s.closing());
    CHECK(feed_all(s, std::string(2000, '\x7f')) && !s.closing());
  }
  // Bytes totales por sesión: se cierra aunque ninguna línea llegue al límite.
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    s.clear_output();
    take(bus);
    std::string burn;
    for (size_t n = 0; n < TelnetMaxSessionBytes / 2 + 1; ++n)
      burn += std::string("a\x08");
    CHECK(!feed_all(s, burn));
    CHECK(s.closing() && s.close_reason() == Close::Limit);
    CHECK(out_of(s) == "\r\nSession limit reached\r\n" && take(bus).empty());
  }
  // El peor caso de salida pendiente (saludo, 16 respuestas de negociación y
  // tres intentos sin vaciar el buffer) cabe en su capacidad y termina por el
  // límite de intentos.
  {
    InteractionBus bus;
    TelnetSession s;
    s.open(telnet_peer(), 100, bus);
    std::string flood;
    for (int n = 0; n < 16; ++n)
      flood += raw({255, 253, 10 + n});
    for (int n = 0; n < 6; ++n)
      flood += "a\r\n";
    feed_all(s, flood);
    CHECK(s.closing() && s.close_reason() == Close::Limit);
    CHECK(s.output_length() <= 256);
  }
}

static void telnet_idle() {
  InteractionBus bus;
  TelnetSession s;
  s.open(telnet_peer(), 1000, bus);
  take(bus);
  CHECK(!s.idle(5999, IdleTimeoutMs) && s.idle(6000, IdleTimeoutMs));
  CHECK(feed_all(s, "a", 5000)); // la actividad reinicia el plazo
  CHECK(!s.idle(9999, IdleTimeoutMs) && s.idle(10000, IdleTimeoutMs));
  s.finish(Close::IdleTimeout, 10000);
  auto closed = take(bus);
  CHECK(closed.size() == 1 && closed[0].kind == Kind::Disconnect &&
        closed[0].close == Close::IdleTimeout);
  s.finish(Close::IdleTimeout, 10001);
  CHECK(take(bus).empty() && !s.idle(99999, IdleTimeoutMs) && !feed_all(s, "a"));
  // Sin motivo explícito la desconexión se atribuye al origen.
  TelnetSession t;
  t.open(telnet_peer(), 1, bus);
  take(bus);
  t.finish(Close::None, 2);
  auto peer_closed = take(bus);
  CHECK(peer_closed.size() == 1 && peer_closed[0].close == Close::Peer);
}

static void telnet_privacy() {
  InteractionBus bus;
  TelnetSession s;
  s.open(telnet_peer(), 100, bus);
  std::string output = out_of(s);
  // Marcador fragmentado en usuario, contraseña, negociación y línea excesiva.
  CHECK(feed_each(s, std::string("user-") + Marker + "\r\n"));
  output += out_of(s);
  CHECK(feed_each(s, raw({255, 250, 24, 0}) + Marker + raw({255, 240})));
  CHECK(feed_each(s, std::string("pass-") + Marker + "\r\n"));
  output += out_of(s);
  CHECK(feed_each(s, std::string(Marker) + "\r\n" + Marker + "\r\n"));
  output += out_of(s);
  CHECK(!feed_each(s, std::string(Marker) + "\r\n" + Marker + "\r\n"));
  output += out_of(s);
  CHECK(s.closing());
  s.finish(Close::Limit, 500);
  CHECK(output.find(Marker) == std::string::npos);
  // Ni la sesión ni el bus guardan nada de lo recibido: no hay dónde.
  CHECK(!contains(&s, sizeof(s), Marker));
  CHECK(!contains(&bus, sizeof(bus), Marker));
  for (const auto &i : take(bus)) {
    char details[320];
    CHECK(format_details(i, details, sizeof(details)));
    CHECK(!contains(details, Marker) && !contains(details, "user-") &&
          !contains(details, "pass-"));
  }
  // La salida solo contiene los textos fijos.
  CHECK(output == Banner + PasswordPrompt + LoginFailed + PasswordPrompt +
                      LoginFailed + PasswordPrompt + TooMany);
}

static void telnet_details() {
  char text[320];
  Interaction i = make_telnet(Kind::Connect);
  CHECK(format_details(i, text, sizeof(text)));
  CHECK(!strcmp(text,
                "{\"source_ipv4\":\"192.0.2.2\",\"source_port\":45000,"
                "\"destination_port\":23,\"protocol\":\"telnet\","
                "\"interaction\":\"connect\",\"path\":null,\"count\":1,"
                "\"window_ms\":1000,\"suspicion\":\"informational\","
                "\"evidence\":[\"connection_observed\"]}"));
  i.kind = Kind::Input;
  CHECK(format_details(i, text, sizeof(text)) &&
        strstr(text, "\"interaction\":\"input\"") &&
        strstr(text, "\"evidence\":[\"input_observed\"]"));
  i.kind = Kind::AuthAttempt;
  CHECK(format_details(i, text, sizeof(text)) &&
        strstr(text, "\"evidence\":[\"auth_attempt\"]"));
  i.kind = Kind::Disconnect;
  i.close = Close::Limit;
  CHECK(format_details(i, text, sizeof(text)) &&
        strstr(text, "\"evidence\":[\"capacity_limit\"]"));
  CHECK(publishable(make_telnet(Kind::Input)) &&
        publishable(make_telnet(Kind::AuthAttempt)) &&
        !publishable(make_telnet(Kind::Connect)) &&
        !publishable(make_telnet(Kind::Disconnect)));
}

static void shared_limits() {
  // 4 de HTTP + 2 de Telnet es el máximo de conexiones del contrato.
  CHECK(HttpMaxSockets + TelnetMaxSessions == MaxConnections && MaxConnections == 6);
  CHECK(TelnetMaxLine == 512 && IdleTimeoutMs == 5000 && TelnetPort == 23);
}

// Muestras reales del código para validarlas con el decodificador compartido
// (check_contracts.mjs): una línea por muestra, etiqueta y JSON.
static void dump() {
  char text[320];
  Interaction http = make(Kind::Connect);
  const struct {
    Kind kind;
    Path path;
    Close close;
  } http_samples[] = {{Kind::Connect, Path::None, Close::None},
                      {Kind::Access, Path::Root, Close::None},
                      {Kind::Access, Path::Admin, Close::None},
                      {Kind::Access, Path::Favicon, Close::None},
                      {Kind::Access, Path::Robots, Close::None},
                      {Kind::Access, Path::Other, Close::None},
                      {Kind::AuthAttempt, Path::Login, Close::None},
                      {Kind::Disconnect, Path::None, Close::Peer},
                      {Kind::Disconnect, Path::None, Close::IdleTimeout},
                      {Kind::Disconnect, Path::None, Close::Limit}};
  for (const auto &s : http_samples) {
    Interaction i = http;
    i.kind = s.kind;
    i.path = s.path;
    i.close = s.close;
    CHECK(format_details(i, text, sizeof(text)));
    std::cout << "details " << text << '\n';
  }
  const struct {
    Kind kind;
    Close close;
  } telnet_samples[] = {{Kind::Connect, Close::None},
                        {Kind::Input, Close::None},
                        {Kind::AuthAttempt, Close::None},
                        {Kind::Disconnect, Close::Peer},
                        {Kind::Disconnect, Close::IdleTimeout},
                        {Kind::Disconnect, Close::Limit}};
  for (const auto &s : telnet_samples) {
    CHECK(format_details(make_telnet(s.kind, s.close), text, sizeof(text)));
    std::cout << "details " << text << '\n';
  }
  // Interacciones producidas por la sesión real, no escritas a mano.
  InteractionBus bus;
  TelnetSession session;
  session.open(telnet_peer(), 100, bus);
  feed_all(session, "root\r\nhunter2\r\n");
  feed_all(session, raw({255, 253, 24}));
  session.finish(Close::Peer, 200);
  for (const auto &i : take(bus)) {
    CHECK(format_details(i, text, sizeof(text)));
    std::cout << "details " << text << '\n';
  }
  FakeCebo p;
  FakeContext ctx;
  Cebo cebo(p);
  CHECK(cebo.init(ctx));
  std::cout << "module " << state_of(cebo) << '\n'; // stopped / stopped
  CHECK(cebo.start());
  std::cout << "module " << state_of(cebo) << '\n'; // starting / starting
  p.lan = true;
  cebo.tick(1000);
  p.h.connections = 3;
  p.t.connections = 1;
  std::cout << "module " << state_of(cebo) << '\n'; // running / running
  p.h.connections = p.t.connections = 99;
  std::cout << "module " << state_of(cebo) << '\n'; // limitado a 6
  FakeCebo broken;
  broken.lan = true;
  broken.t.start_ok = false;
  Cebo partial(broken);
  CHECK(partial.init(ctx) && partial.start());
  partial.tick(1000);
  std::cout << "module " << state_of(partial) << '\n'; // running / error
  FakeCebo failing;
  failing.lan = true;
  failing.h.start_ok = failing.t.start_ok = false;
  Cebo none(failing);
  CHECK(none.init(ctx) && none.start());
  none.tick(1000);
  std::cout << "module " << state_of(none) << '\n'; // error / error
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
  std::cout << "Cebo HTTP privacy (secret marker absent from every output): PASS\n";
  details_format(argc > 1 ? argv[1] : nullptr);
  telnet_details();
  shared_limits();
  telnet_flow();
  telnet_pipelined();
  telnet_line_endings();
  telnet_iac();
  telnet_limits();
  telnet_idle();
  std::cout << "Cebo Telnet session (fragmented input, IAC, limits, idle): PASS\n";
  telnet_privacy();
  std::cout << "Cebo Telnet privacy (secret marker absent; line never stored): PASS\n";
  lifecycle();
  independent_failures();
  std::cout << "Cebo module lifecycle (LAN wait, shared bus, joint stop, restart): PASS\n";
  return 0;
}
