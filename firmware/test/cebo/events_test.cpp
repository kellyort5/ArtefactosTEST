// Pruebas de host de la clasificación, métricas, límites y configuración de
// Cebo (sentinel-cebo-eventos-historial). Lógica pura con reloj controlado: no
// hay radio, sockets ni NVS real. El NVS se simula en memoria, con inyección de
// fallos y confirmación solo tras commit, para probar el almacén real
// (config_store.cpp).
#include "test_support.hpp"
#include "nvs.h" // stub de la frontera del SDK (firmware/test/stubs/cebo)
#include <cmath>

namespace {
bool has(const std::string &text, const char *needle) {
  return text.find(needle) != std::string::npos;
}
constexpr uint64_t T0 = 1000000;

Interaction at(Protocol p, Kind k, uint8_t host, uint64_t t,
               Path path = Path::None, Close close = Close::None) {
  Interaction i;
  i.ipv4[0] = 192;
  i.ipv4[2] = 2;
  i.ipv4[3] = host;
  i.source_port = 45000;
  i.destination_port = p == Protocol::Telnet ? 23 : 80;
  i.protocol = p;
  i.kind = k;
  i.path = path;
  i.close = close;
  i.uptime_ms = t;
  return i;
}
// Clasifica y, si el evento se publicaría, lo confirma como aceptado.
Classification feed(Classifier &c, const Interaction &i) {
  Classification r = c.observe(i);
  c.commit(i, r, r.publish);
  return r;
}
std::string evidence_of(const Classification &c) {
  std::string out;
  for (unsigned n = 0; n < c.evidence_count; ++n)
    out += (n ? "," : "") + std::string(c.evidence[n]);
  return out;
}

// ---- Clasificador ---------------------------------------------------------
void classifier_isolated() {
  Classifier c;
  Classification a = feed(c, at(Protocol::Http, Kind::Access, 2, T0, Path::Root));
  CHECK(a.publish && a.suspicion == Suspicion::Informational && a.count == 1 &&
        a.window_ms == IsolatedWindowMs && evidence_of(a) == "canonical_access");
  CHECK(!strcmp(severity_text(a.suspicion), "info"));
  // Dentro del enfriamiento el acceso repetido no publica, pero se cuenta.
  Classification b =
      feed(c, at(Protocol::Http, Kind::Access, 2, T0 + 1000, Path::Root));
  CHECK(!b.publish && c.suppressed() == 1);
  // Una conexión sola nunca produce evento informativo; el cierre tampoco.
  CHECK(!feed(c, at(Protocol::Http, Kind::Connect, 2, T0 + 2000)).publish);
  CHECK(!feed(c, at(Protocol::Http, Kind::Disconnect, 2, T0 + 2500, Path::None,
                    Close::Peer))
             .publish);
  CHECK(c.suppressed() == 1); // solo cuentan las interacciones publicables
  // Otro origen y otro protocolo del mismo origen tienen su propia entrada.
  CHECK(feed(c, at(Protocol::Http, Kind::Access, 3, T0 + 1000, Path::Root)).publish);
  CHECK(feed(c, at(Protocol::Telnet, Kind::Input, 2, T0 + 1000)).publish);
  CHECK(c.size() == 3);
  // Pasado el enfriamiento vuelve a publicar.
  CHECK(feed(c, at(Protocol::Http, Kind::Access, 2, T0 + CooldownMs - 1, Path::Root))
            .publish == false);
  CHECK(feed(c, at(Protocol::Http, Kind::Access, 2, T0 + CooldownMs, Path::Root))
            .publish);
  // El cierre no suma actividad: la conexión sigue con 1 conexión y 3 accesos.
  CHECK(feed(c, at(Protocol::Http, Kind::Disconnect, 2, T0 + CooldownMs + 1,
                   Path::None, Close::IdleTimeout))
            .publish == false);
}

void classifier_repeated_auth() {
  Classifier c;
  std::vector<Classification> published;
  for (unsigned n = 0; n < 5; ++n) {
    Classification r =
        feed(c, at(Protocol::Http, Kind::AuthAttempt, 2, T0 + n * 1000, Path::Login));
    if (r.publish)
      published.push_back(r);
    if (n == 0)
      CHECK(r.publish && r.suspicion == Suspicion::Informational);
    else if (n < 4)
      CHECK(!r.publish); // aislado/repetido por debajo del umbral: enfriamiento
  }
  // El quinto intento escala a medio aunque el origen esté en enfriamiento.
  CHECK(published.size() == 2);
  const Classification &m = published[1];
  CHECK(m.suspicion == Suspicion::Medium && m.count == 5 &&
        m.window_ms == WindowMs && evidence_of(m) == "auth_attempt,repeated_auth");
  CHECK(!strcmp(suspicion_text(m.suspicion), "medium") &&
        !strcmp(severity_text(m.suspicion), "warning"));
  // Más intentos: suprimidos hasta que pase el enfriamiento desde la alerta.
  const uint64_t alerted_at = T0 + 4000;
  CHECK(!feed(c, at(Protocol::Http, Kind::AuthAttempt, 2, alerted_at + 1000, Path::Login))
             .publish);
  Classification again = feed(
      c, at(Protocol::Http, Kind::AuthAttempt, 2, alerted_at + CooldownMs, Path::Login));
  CHECK(again.publish && again.suspicion == Suspicion::Medium && again.count == 7);
}

void classifier_rate_threshold() {
  Classifier c;
  std::vector<std::pair<Classification, Kind>> published;
  uint64_t t = T0;
  auto step = [&](Kind k) {
    Classification r = feed(c, at(Protocol::Telnet, k, 5, t));
    t += 100;
    if (r.publish)
      published.push_back({r, k});
    return r;
  };
  step(Kind::Connect);
  Classification last;
  for (unsigned pair = 1; pair <= 10 && last.suspicion != Suspicion::High; ++pair) {
    step(Kind::Input);
    last = step(Kind::AuthAttempt);
  }
  // Input #10 alcanza 20 interacciones (1 conexión + 9 pares + la entrada).
  CHECK(!published.empty());
  const auto &high = published.back();
  CHECK(high.first.suspicion == Suspicion::High && high.second == Kind::Input &&
        high.first.count == 20 && high.first.window_ms == WindowMs &&
        evidence_of(high.first) == "input_observed,rate_threshold");
  CHECK(!strcmp(severity_text(high.first.suspicion), "critical"));
  // Antes hubo: informativo (primera entrada) y medio (quinto intento), cada
  // escalada dentro del enfriamiento.
  CHECK(published.size() == 3 &&
        published[0].first.suspicion == Suspicion::Informational &&
        published[1].first.suspicion == Suspicion::Medium &&
        published[1].first.count == 5 && published[1].second == Kind::AuthAttempt);
  // Ya en nivel alto: nada más hasta el enfriamiento.
  CHECK(!step(Kind::Input).publish);
}

void classifier_connection_flood() {
  // Un barrido de conexiones sin peticiones tampoco pasa inadvertido.
  Classifier c;
  std::vector<Classification> published;
  for (unsigned n = 0; n < 25; ++n) {
    Classification r = feed(c, at(Protocol::Http, Kind::Connect, 6, T0 + n * 100));
    if (r.publish)
      published.push_back(r);
    else
      CHECK(n != 19); // la conexión número 20 sube de nivel y publica
  }
  CHECK(published.size() == 1 && published[0].suspicion == Suspicion::High &&
        published[0].count == 20 &&
        evidence_of(published[0]) == "connection_observed,rate_threshold");
}

void classifier_windows_and_expiry() {
  Classifier c;
  // Cuatro intentos y, tras la ventana, otros cuatro: nunca llegan a cinco.
  for (unsigned n = 0; n < 4; ++n)
    feed(c, at(Protocol::Http, Kind::AuthAttempt, 7, T0 + n * 1000, Path::Login));
  Classification first_after_window = feed(
      c, at(Protocol::Http, Kind::AuthAttempt, 7, T0 + WindowMs, Path::Login));
  CHECK(first_after_window.publish &&
        first_after_window.suspicion == Suspicion::Informational);
  for (unsigned n = 1; n < 4; ++n)
    CHECK(feed(c, at(Protocol::Http, Kind::AuthAttempt, 7, T0 + WindowMs + n * 1000,
                     Path::Login))
              .suspicion == Suspicion::Informational);
  // Un instante antes del final de la ventana todavía cuenta en ella.
  Classifier edge;
  for (unsigned n = 0; n < 4; ++n)
    feed(edge, at(Protocol::Http, Kind::AuthAttempt, 7, T0, Path::Login));
  CHECK(feed(edge, at(Protocol::Http, Kind::AuthAttempt, 7, T0 + WindowMs - 1,
                      Path::Login))
            .suspicion == Suspicion::Medium);
  // El nivel publicado se reinicia con la ventana: una alerta nueva vuelve a
  // publicarse aunque la anterior fuera del mismo nivel.
  Classifier again;
  for (unsigned n = 0; n < 5; ++n)
    feed(again, at(Protocol::Http, Kind::AuthAttempt, 8, T0 + n * 100, Path::Login));
  const uint64_t later = T0 + WindowMs + 10;
  for (unsigned n = 0; n < 4; ++n)
    feed(again, at(Protocol::Http, Kind::AuthAttempt, 8, later + n * 100, Path::Login));
  Classification second = feed(
      again, at(Protocol::Http, Kind::AuthAttempt, 8, later + 400, Path::Login));
  CHECK(second.publish && second.suspicion == Suspicion::Medium &&
        second.count == 5);

  // Expiración: sin actividad durante ExpiryMs el origen se olvida.
  Classifier e;
  feed(e, at(Protocol::Http, Kind::Access, 9, T0, Path::Root));
  CHECK(e.size() == 1);
  e.expire(T0 + ExpiryMs - 1);
  CHECK(e.size() == 1);
  e.expire(T0 + ExpiryMs);
  CHECK(e.size() == 0);
  // Y observe también expira lo antiguo antes de contar.
  feed(e, at(Protocol::Http, Kind::Access, 9, T0, Path::Root));
  feed(e, at(Protocol::Http, Kind::Access, 10, T0 + ExpiryMs, Path::Root));
  CHECK(e.size() == 1);
  // Un origen caducado vuelve a ser aislado: publica de nuevo.
  CHECK(feed(e, at(Protocol::Http, Kind::Access, 9, T0 + ExpiryMs + 5, Path::Root))
            .publish);
  // Un reloj que retrocede no rompe la ventana ni desborda restas.
  Classifier skew;
  feed(skew, at(Protocol::Http, Kind::Access, 11, T0 + 5000, Path::Root));
  Classification back =
      feed(skew, at(Protocol::Http, Kind::Access, 11, T0, Path::Root));
  CHECK(!back.publish && skew.size() == 1);
}

void classifier_capacity() {
  Classifier c;
  for (unsigned n = 1; n <= ClassifierEntries; ++n)
    CHECK(feed(c, at(Protocol::Http, Kind::Access, uint8_t(n), T0 + n * 10, Path::Root))
              .publish);
  CHECK(c.size() == ClassifierEntries && c.evictions() == 0);
  // El origen 33 desaloja al visto hace más tiempo (el 1) y conserva a los demás.
  CHECK(feed(c, at(Protocol::Http, Kind::Access, 33, T0 + 1000, Path::Root)).publish);
  CHECK(c.size() == ClassifierEntries && c.evictions() == 1);
  CHECK(!feed(c, at(Protocol::Http, Kind::Access, 20, T0 + 1100, Path::Root)).publish);
  // El origen 1 es nuevo otra vez (publica) y desaloja al siguiente (el 2).
  CHECK(feed(c, at(Protocol::Http, Kind::Access, 1, T0 + 1200, Path::Root)).publish);
  CHECK(c.evictions() == 2);

  // Una confirmación tardía de un origen ya desalojado no afecta a quien ocupa
  // su lugar.
  Classifier stale;
  Interaction first = at(Protocol::Http, Kind::Access, 1, T0, Path::Root);
  Classification pending = stale.observe(first);
  CHECK(pending.publish);
  for (unsigned n = 2; n <= ClassifierEntries; ++n)
    feed(stale, at(Protocol::Http, Kind::Access, uint8_t(n), T0 + n, Path::Root));
  Interaction newcomer = at(Protocol::Http, Kind::Access, 40, T0 + 100, Path::Root);
  Classification n40 = stale.observe(newcomer); // desaloja al origen 1
  CHECK(n40.publish && n40.slot == pending.slot);
  stale.commit(first, pending, true); // tardía: debe ignorarse
  CHECK(feed(stale, at(Protocol::Http, Kind::Access, 40, T0 + 200, Path::Root))
            .publish); // nunca se confirmó el suyo: no está en enfriamiento
}

void classifier_unconfirmed() {
  Classifier c;
  Interaction first = at(Protocol::Http, Kind::Access, 12, T0, Path::Root);
  Classification r = c.observe(first);
  CHECK(r.publish);
  c.commit(first, r, false); // el núcleo no lo aceptó
  // Sin confirmación no hay enfriamiento: el siguiente acceso publica.
  Interaction second = at(Protocol::Http, Kind::Access, 12, T0 + 500, Path::Root);
  Classification r2 = c.observe(second);
  CHECK(r2.publish);
  c.commit(second, r2, true);
  CHECK(!feed(c, at(Protocol::Http, Kind::Access, 12, T0 + 1000, Path::Root)).publish);
  // Una alerta no confirmada se vuelve a intentar con la siguiente interacción.
  Classifier a;
  for (unsigned n = 0; n < 4; ++n)
    feed(a, at(Protocol::Http, Kind::AuthAttempt, 13, T0 + n, Path::Login));
  Interaction fifth = at(Protocol::Http, Kind::AuthAttempt, 13, T0 + 4, Path::Login);
  Classification alert = a.observe(fifth);
  CHECK(alert.publish && alert.suspicion == Suspicion::Medium);
  a.commit(fifth, alert, false);
  Interaction sixth = at(Protocol::Http, Kind::AuthAttempt, 13, T0 + 5, Path::Login);
  Classification retried = a.observe(sixth);
  CHECK(retried.publish && retried.suspicion == Suspicion::Medium &&
        retried.count == 6);
}

// ---- Métricas ---------------------------------------------------------------
void metrics_values() {
  Metrics m;
  CHECK(m.alerts() == 0);
  m.record(at(Protocol::Http, Kind::Connect, 2, 1000));
  m.record(at(Protocol::Http, Kind::Access, 2, 1000, Path::Root));
  m.record(at(Protocol::Http, Kind::Access, 2, 1500, Path::Admin));
  m.record(at(Protocol::Http, Kind::AuthAttempt, 2, 1600, Path::Login));
  m.record(at(Protocol::Http, Kind::Input, 2, 1700));
  m.record(at(Protocol::Telnet, Kind::Connect, 2, 1800));
  m.record(at(Protocol::Http, Kind::Disconnect, 2, 1900, Path::None, Close::Peer));
  char text[512];
  CHECK(m.format(text, sizeof(text), 2, 1, 3, 2000));
  CHECK(std::string(text) ==
        "{\"http\":{\"connections\":1,\"accesses\":2,\"auth_attempts\":1,"
        "\"inputs\":1,\"active_connections\":2,\"interactions_per_minute\":5,"
        "\"alerts\":0},\"telnet\":{\"connections\":1,\"accesses\":0,"
        "\"auth_attempts\":0,\"inputs\":0,\"active_connections\":1,"
        "\"interactions_per_minute\":1,\"alerts\":0},"
        "\"rejected_connections\":3,\"alerts\":0}");
  m.alert(Protocol::Http);
  m.alert(Protocol::Http);
  m.alert(Protocol::Telnet);
  CHECK(m.alerts() == 3);
  CHECK(m.format(text, sizeof(text), 0, 0, 0, 2000));
  CHECK(has(text, "\"auth_attempts\":1,\"inputs\":1,\"active_connections\":0,"
                  "\"interactions_per_minute\":5,\"alerts\":2}") &&
        has(text, "\"rejected_connections\":0,\"alerts\":3}"));
  // El contrato limita active_connections a 6.
  CHECK(m.format(text, sizeof(text), 99, 99, 0, 2000));
  CHECK(has(text, "\"active_connections\":6,") && !has(text, "99"));
  // Un búfer insuficiente no deja un JSON truncado.
  char small[40];
  CHECK(!m.format(small, sizeof(small), 0, 0, 0, 2000) && !*small);
  CHECK(!m.format(nullptr, 0, 0, 0, 0, 2000));
  m.reset();
  CHECK(m.connections(Protocol::Http) == 0 && m.alerts() == 0);
}

void metrics_rate_window() {
  Metrics m;
  for (unsigned n = 0; n < 3; ++n)
    m.record(at(Protocol::Http, Kind::Access, 2, 10000 + n, Path::Root));
  for (unsigned n = 0; n < 2; ++n)
    m.record(at(Protocol::Http, Kind::Access, 2, 69000 + n, Path::Root));
  CHECK(m.per_minute(Protocol::Http, 69000) == 5);
  CHECK(m.per_minute(Protocol::Http, 69999) == 5);
  // El segundo 10 sale de la ventana de 60 s en el segundo 70.
  CHECK(m.per_minute(Protocol::Http, 70000) == 2);
  CHECK(m.per_minute(Protocol::Http, 129000) == 0);
  CHECK(m.per_minute(Protocol::Telnet, 69000) == 0);
  // Un segundo que reutiliza el cubo del 10 (70 % 60) no hereda su cuenta.
  Metrics reuse;
  for (unsigned n = 0; n < 3; ++n)
    reuse.record(at(Protocol::Http, Kind::Access, 2, 10000, Path::Root));
  reuse.record(at(Protocol::Http, Kind::Access, 2, 70000, Path::Root));
  CHECK(reuse.per_minute(Protocol::Http, 70000) == 1);
  CHECK(reuse.accesses(Protocol::Http) == 4);
}

// ---- Límites ----------------------------------------------------------------
void limits() {
  CHECK(valid(Config()));
  CHECK(valid(Config{1, MinIdleTimeoutMs}) &&
        valid(Config{MaxConnections, MaxIdleTimeoutMs}));
  CHECK(!valid(Config{0, IdleTimeoutMs}) &&
        !valid(Config{MaxConnections + 1, IdleTimeoutMs}) &&
        !valid(Config{3, MinIdleTimeoutMs - 1}) &&
        !valid(Config{3, MaxIdleTimeoutMs + 1}));

  Limiter l;
  CHECK(l.config() == Config() && l.idle_timeout_ms() == IdleTimeoutMs);
  // Máximo del contrato: 4 de HTTP + 2 de Telnet; cada servicio con su tope.
  for (unsigned n = 0; n < HttpMaxSockets; ++n)
    CHECK(l.admit(Protocol::Http));
  CHECK(!l.admit(Protocol::Http) && l.rejected() == 1);
  for (unsigned n = 0; n < TelnetMaxSessions; ++n)
    CHECK(l.admit(Protocol::Telnet));
  CHECK(!l.admit(Protocol::Telnet) && l.rejected() == 2 && l.active() == 6 &&
        l.active(Protocol::Http) == 4 && l.active(Protocol::Telnet) == 2);
  l.release(Protocol::Http);
  CHECK(l.active() == 5 && l.admit(Protocol::Http) && l.active() == 6);
  // Un valor menor limita el total entre los dos servicios, sin cortar las
  // conexiones abiertas.
  l.configure(Config{3, 20000});
  CHECK(l.config().max_connections == 3 && l.idle_timeout_ms() == 20000);
  CHECK(l.active() == 6 && !l.admit(Protocol::Http));
  l.reset(Protocol::Http);
  l.reset(Protocol::Telnet);
  CHECK(l.active() == 0);
  CHECK(l.admit(Protocol::Telnet) && l.admit(Protocol::Telnet) &&
        l.admit(Protocol::Http) && !l.admit(Protocol::Http) &&
        !l.admit(Protocol::Telnet));
  // release de más nunca baja de cero.
  l.reset(Protocol::Http);
  l.release(Protocol::Http);
  CHECK(l.active(Protocol::Http) == 0);
  l.clear();
  CHECK(l.active() == 0 && l.rejected() == 0 && l.config().max_connections == 3);
}

// ---- Almacén NVS (config_store.cpp) con NVS simulado --------------------------
namespace fake {
struct State {
  bool present = false;
  std::vector<uint8_t> committed, pending;
  bool fail_open_read = false, fail_open_write = false, fail_size = false,
       fail_read = false, fail_set = false, fail_commit = false,
       wrong_size = false;
  unsigned opens = 0, handles = 0, sets = 0, commits = 0;
  int mode = NVS_READONLY;
} nvs;
void reset() { nvs = State(); }
} // namespace fake
} // namespace

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle) {
  CHECK(!strcmp(name, "cebo_config")); // nunca otro espacio de nombres
  ++fake::nvs.opens;
  if ((mode == NVS_READONLY && fake::nvs.fail_open_read) ||
      (mode == NVS_READWRITE && fake::nvs.fail_open_write))
    return ESP_FAIL;
  if (mode == NVS_READONLY && !fake::nvs.present)
    return ESP_ERR_NVS_NOT_FOUND;
  fake::nvs.mode = mode;
  fake::nvs.pending.clear();
  ++fake::nvs.handles;
  *handle = 7;
  return ESP_OK;
}
void nvs_close(nvs_handle_t handle) {
  CHECK(handle == 7 && fake::nvs.handles > 0);
  --fake::nvs.handles;
  fake::nvs.pending.clear(); // lo no confirmado se pierde, como un corte
}
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out,
                       size_t *size) {
  CHECK(handle == 7 && !strcmp(key, "settings"));
  if (fake::nvs.committed.empty())
    return ESP_ERR_NVS_NOT_FOUND;
  if ((!out && fake::nvs.fail_size) || (out && fake::nvs.fail_read))
    return ESP_FAIL;
  if (out) {
    CHECK(*size >= fake::nvs.committed.size());
    memcpy(out, fake::nvs.committed.data(), fake::nvs.committed.size());
  }
  *size = fake::nvs.committed.size() + (out && fake::nvs.wrong_size ? 1 : 0);
  return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *data,
                       size_t size) {
  CHECK(handle == 7 && fake::nvs.mode == NVS_READWRITE &&
        !strcmp(key, "settings") && size == 13);
  ++fake::nvs.sets;
  if (fake::nvs.fail_set)
    return ESP_FAIL;
  fake::nvs.pending.assign(static_cast<const uint8_t *>(data),
                           static_cast<const uint8_t *>(data) + size);
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle) {
  CHECK(handle == 7);
  ++fake::nvs.commits;
  if (fake::nvs.fail_commit)
    return ESP_FAIL;
  if (!fake::nvs.pending.empty())
    fake::nvs.committed = fake::nvs.pending;
  fake::nvs.present = true;
  return ESP_OK;
}

namespace {
void store_persistence() {
  ConfigStore &store = nvs_config_store();
  Config out = Config{2, 1234};
  fake::reset();
  // Nunca configurado.
  CHECK(store.load(out) == Load::Missing && out == (Config{2, 1234}));
  // Guardar y volver a leer (reinicio simulado: el almacén no guarda estado).
  CHECK(store.save(Config{4, 20000}));
  CHECK(fake::nvs.sets == 1 && fake::nvs.commits == 1 && fake::nvs.handles == 0);
  CHECK(store.load(out) == Load::Loaded && out == (Config{4, 20000}));
  // El registro contiene solo cabecera y los dos valores, sin texto libre.
  CHECK(fake::nvs.committed.size() == 13 &&
        !memcmp(fake::nvs.committed.data(), "SCEB\1\0\0\0", 8) &&
        fake::nvs.committed[8] == 4 && fake::nvs.committed[9] == (20000 & 0xFF) &&
        fake::nvs.committed[10] == ((20000 >> 8) & 0xFF) &&
        fake::nvs.committed[11] == 0 && fake::nvs.committed[12] == 0);

  // Un valor inválido no llega a tocar NVS.
  unsigned opens = fake::nvs.opens;
  CHECK(!store.save(Config{0, 5000}) && !store.save(Config{7, 5000}) &&
        !store.save(Config{3, 999}) && !store.save(Config{3, 60001}) &&
        fake::nvs.opens == opens);

  // Fallos al escribir: se conserva la configuración anterior.
  fake::nvs.fail_open_write = true;
  CHECK(!store.save(Config{1, 1000}));
  fake::nvs.fail_open_write = false;
  fake::nvs.fail_set = true;
  CHECK(!store.save(Config{1, 1000}));
  fake::nvs.fail_set = false;
  fake::nvs.fail_commit = true;
  CHECK(!store.save(Config{1, 1000}));
  fake::nvs.fail_commit = false;
  out = Config();
  CHECK(store.load(out) == Load::Loaded && out == (Config{4, 20000}));
  // Un corte entre la escritura y el commit tampoco cambia lo confirmado: el
  // pendiente se descarta al cerrar.
  CHECK(fake::nvs.pending.empty());

  // Errores de lectura: se informan sin borrar nada.
  fake::nvs.fail_open_read = true;
  CHECK(store.load(out) == Load::Error);
  fake::nvs.fail_open_read = false;
  fake::nvs.fail_size = true;
  CHECK(store.load(out) == Load::Error);
  fake::nvs.fail_size = false;
  fake::nvs.fail_read = true;
  CHECK(store.load(out) == Load::Error);
  fake::nvs.fail_read = false;
  fake::nvs.wrong_size = true;
  CHECK(store.load(out) == Load::Error);
  fake::nvs.wrong_size = false;
  CHECK(out == (Config{4, 20000}) && fake::nvs.committed.size() == 13);

  // Registros dañados o ajenos nunca se aplican.
  const std::vector<uint8_t> good = fake::nvs.committed;
  auto expect_error = [&](std::vector<uint8_t> record) {
    fake::nvs.committed = record;
    Config untouched{5, 9999};
    Config probe = untouched;
    CHECK(store.load(probe) == Load::Error && probe == untouched);
    fake::nvs.committed = good;
  };
  auto with = [&](uint8_t max, uint32_t idle) {
    std::vector<uint8_t> record = good;
    record[8] = max;
    for (unsigned n = 0; n < 4; ++n)
      record[9 + n] = uint8_t(idle >> (8 * n));
    return record;
  };
  CHECK(with(4, 20000) == good);
  std::vector<uint8_t> record = good;
  record[0] = 'X'; // cabecera ajena
  expect_error(record);
  record = good;
  record[4] = 2; // versión desconocida
  expect_error(record);
  expect_error(with(0, 20000)); // max_connections = 0
  expect_error(with(7, 20000)); // max_connections > 6
  expect_error(with(4, 0));     // idle_timeout_ms = 0
  expect_error(with(4, 999));   // por debajo del mínimo
  expect_error(with(4, 60001)); // por encima del máximo
  record = good;
  record.resize(12); // tamaño inesperado
  expect_error(record);
  record = good;
  record.push_back(0);
  expect_error(record);
  CHECK(store.load(out) == Load::Loaded && out == (Config{4, 20000}));
  // Todo se cerró sin dejar manejadores.
  CHECK(fake::nvs.handles == 0);
}

// ---- Módulo: configuración, estadísticas y eventos --------------------------
struct Args {
  cJSON root, http, telnet, max, idle;
  explicit Args(bool http_on = true, bool telnet_on = true, double max_value = 6,
                double idle_value = 5000) {
    node(http, "http_enabled", http_on ? cJSON_True : cJSON_False, 0);
    node(telnet, "telnet_enabled", telnet_on ? cJSON_True : cJSON_False, 0);
    node(max, "max_connections", cJSON_Number, max_value);
    node(idle, "idle_timeout_ms", cJSON_Number, idle_value);
    root.type = cJSON_Object;
    root.child = &http;
    http.next = &telnet;
    telnet.next = &max;
    max.next = &idle;
  }
  Args(const Args &) = delete; // los nodos se enlazan entre sí
  Args &operator=(const Args &) = delete;
  static void node(cJSON &n, const char *name, int type, double value) {
    n.string = const_cast<char *>(name);
    n.type = type;
    n.valuedouble = value;
    n.valueint = std::isfinite(value) && std::fabs(value) < 2e9 ? int(value) : 0;
  }
};

std::string state_with(const char *http, const char *telnet, unsigned active,
                       unsigned max, unsigned idle) {
  char text[420];
  snprintf(text, sizeof(text),
           "{\"kind\":\"cebo\",\"http\":{\"available\":true,\"status\":\"%s\"},"
           "\"telnet\":{\"available\":true,\"status\":\"%s\"},"
           "\"active_connections\":%u,\"max_connections\":%u,"
           "\"idle_timeout_ms\":%u}",
           http, telnet, active, max, idle);
  return text;
}

struct Rig {
  FakeCebo p;
  FakeContext ctx;
  Cebo cebo;
  Rig() : cebo(p) {
    ctx.now = 1000;
    CHECK(cebo.init(ctx));
  }
  ModuleReply command(const char *operation, const cJSON *args = nullptr) {
    ModuleReply reply;
    cebo.handle({operation, args, "cmd"}, reply);
    return reply;
  }
  bool push(const Interaction &i) { return cebo.bus().push(i); }
};

void configure_command() {
  Rig r;
  // Antes de entrar en el modo no hay configuración que cambiar.
  Args any;
  CHECK(r.command("cebo.configure", &any.root).error &&
        !strcmp(r.command("cebo.configure", &any.root).error, "invalid_state"));
  CHECK(r.cebo.start());
  CHECK(r.p.store.loads == 1);
  CHECK(state_of(r.cebo) == state_with("starting", "starting", 0, 6, 5000));

  Args ok(true, true, 3, 20000);
  ModuleReply reply = r.command("cebo.configure", &ok.root);
  CHECK(!reply.error && !*reply.result && !reply.job);
  CHECK(r.p.store.saves == 1 && r.p.store.stored == (Config{3, 20000}));
  CHECK(r.cebo.config() == (Config{3, 20000}) &&
        r.cebo.limiter().config() == (Config{3, 20000}));
  CHECK(state_of(r.cebo) == state_with("starting", "starting", 0, 3, 20000));
  // Surte efecto en los servicios sin reiniciarlos: el límite compartido.
  Limiter &l = r.cebo.limiter();
  CHECK(l.admit(Protocol::Http) && l.admit(Protocol::Http) &&
        l.admit(Protocol::Telnet) && !l.admit(Protocol::Http) &&
        l.idle_timeout_ms() == 20000);
  // La misma configuración no vuelve a escribir en flash.
  reply = r.command("cebo.configure", &ok.root);
  CHECK(!reply.error && r.p.store.saves == 1);

  // Argumentos inválidos: ni se guardan ni cambian la configuración vigente.
  struct Bad {
    const char *what;
    Args args;
  } bad[] = {{"http_enabled=false", Args(false, true, 3, 20000)},
             {"telnet_enabled=false", Args(true, false, 3, 20000)},
             {"max=0", Args(true, true, 0, 20000)},
             {"max=7", Args(true, true, 7, 20000)},
             {"max=3.5", Args(true, true, 3.5, 20000)},
             {"max=-1", Args(true, true, -1, 20000)},
             {"idle=999", Args(true, true, 3, 999)},
             {"idle=60001", Args(true, true, 3, 60001)},
             {"idle=1500.5", Args(true, true, 3, 1500.5)},
             {"idle=NaN", Args(true, true, 3, std::nan(""))}};
  for (auto &b : bad) {
    reply = r.command("cebo.configure", &b.args.root);
    if (!reply.error || strcmp(reply.error, "invalid_args")) {
      std::fprintf(stderr, "invalid args accepted: %s\n", b.what);
      std::exit(1);
    }
  }
  reply = r.command("cebo.configure", nullptr);
  CHECK(reply.error && !strcmp(reply.error, "invalid_args"));
  Args wrong_type(true, true, 3, 20000);
  wrong_type.max.type = cJSON_String;
  reply = r.command("cebo.configure", &wrong_type.root);
  CHECK(reply.error && !strcmp(reply.error, "invalid_args"));
  Args missing(true, true, 3, 20000);
  missing.telnet.next = nullptr; // sin idle_timeout_ms ni max_connections
  reply = r.command("cebo.configure", &missing.root);
  CHECK(reply.error && !strcmp(reply.error, "invalid_args"));
  CHECK(r.p.store.saves == 1 && r.cebo.config() == (Config{3, 20000}));

  // Un fallo del almacén se informa y conserva lo anterior, también en RAM.
  r.p.store.save_ok = false;
  Args next(true, true, 5, 8000);
  reply = r.command("cebo.configure", &next.root);
  CHECK(reply.error && !strcmp(reply.error, "resource_error"));
  CHECK(r.cebo.config() == (Config{3, 20000}) &&
        r.cebo.limiter().config() == (Config{3, 20000}) &&
        r.p.store.stored == (Config{3, 20000}));
  r.p.store.save_ok = true;
  reply = r.command("cebo.configure", &next.root);
  CHECK(!reply.error && r.cebo.config() == (Config{5, 8000}));
}

void restore_command() {
  // Reinicio: una configuración confirmada se restaura al entrar en el modo,
  // sin abrir ningún socket por sí misma.
  FakeCebo p;
  FakeContext ctx;
  ctx.now = 1000;
  p.store.load_result = Load::Loaded;
  p.store.stored = Config{2, 30000};
  Cebo before(p);
  CHECK(before.init(ctx));
  // En Idle (antes de start) el módulo no lee ni aplica nada.
  CHECK(p.store.loads == 0 && state_of(before) == state_with("stopped", "stopped", 0, 6, 5000));
  CHECK(before.start());
  CHECK(p.store.loads == 1 && before.config() == (Config{2, 30000}) &&
        before.limiter().config() == (Config{2, 30000}));
  CHECK(state_of(before) == state_with("starting", "starting", 0, 2, 30000));
  CHECK(p.h.starts == 0 && p.t.starts == 0 && !p.lan); // sin LAN no abre nada
  before.request_stop();
  CHECK(before.stopped());

  // Configurar, "reiniciar" (otra instancia con el mismo almacén) y restaurar.
  FakeCebo q;
  Cebo first(q);
  CHECK(first.init(ctx) && first.start());
  Args args(true, true, 4, 12000);
  ModuleReply reply;
  first.handle({"cebo.configure", &args.root, "c"}, reply);
  CHECK(!reply.error);
  Cebo rebooted(q);
  CHECK(rebooted.init(ctx) && rebooted.start());
  CHECK(rebooted.config() == (Config{4, 12000}) &&
        state_of(rebooted) == state_with("starting", "starting", 0, 4, 12000));

  // Un registro dañado se informa una vez (con reintento) y se usan los
  // valores por defecto, sin borrar nada.
  FakeCebo broken;
  broken.store.load_result = Load::Error;
  Cebo c(broken);
  CHECK(c.init(ctx) && c.start());
  CHECK(c.config() == Config() &&
        state_of(c) == state_with("starting", "starting", 0, 6, 5000));
  ctx.accept = false;
  c.tick(1100);
  CHECK(ctx.events.empty());
  ctx.accept = true;
  c.tick(1200);
  CHECK(ctx.events.size() == 1 && ctx.events[0].name == "system.status" &&
        ctx.events[0].severity == "warning" && !ctx.events[0].simulated &&
        ctx.events[0].details ==
            "{\"status_code\":\"resource_error\",\"resource\":\"system\"}");
  c.tick(1300);
  CHECK(ctx.events.size() == 1);
  // Una selección explícita válida corrige el estado.
  Args fix(true, true, 6, 5000);
  ModuleReply fixed;
  c.handle({"cebo.configure", &fix.root, "d"}, fixed);
  // El registro dañado se sobrescribe aunque coincida con los valores por
  // defecto; a partir de ahí se considera confirmado y no se vuelve a escribir.
  CHECK(!fixed.error && broken.store.saves == 1 &&
        broken.store.stored == Config() &&
        broken.store.load_result == Load::Loaded);
  c.handle({"cebo.configure", &fix.root, "e"}, fixed);
  CHECK(!fixed.error && broken.store.saves == 1);
}

void events_flow() {
  Rig r;
  CHECK(r.cebo.start());
  r.p.lan = true;
  r.cebo.tick(1100);
  // Cinco intentos de login desde un origen: primero informativo y, al quinto,
  // medio. Los demás cuentan pero no producen evento.
  for (unsigned n = 0; n < 5; ++n) {
    Interaction i = at(Protocol::Http, Kind::AuthAttempt, 2, 1100 + n * 100, Path::Login);
    i.source_port = uint16_t(40000 + n);
    CHECK(r.push(i));
  }
  r.cebo.tick(1700);
  CHECK(r.ctx.events.size() == 2);
  const Emitted &first = r.ctx.events[0], &alert = r.ctx.events[1];
  CHECK(first.name == "cebo.interaction" && first.severity == "info" && !first.simulated &&
        has(first.details, "\"source_port\":40000") &&
        has(first.details, "\"count\":1,\"window_ms\":1000,\"suspicion\":\"informational\","
                           "\"evidence\":[\"auth_attempt\"]}"));
  CHECK(alert.severity == "warning" && !alert.simulated &&
        has(alert.details, "\"source_port\":40004") &&
        has(alert.details, "\"count\":5,\"window_ms\":60000,\"suspicion\":\"medium\","
                           "\"evidence\":[\"auth_attempt\",\"repeated_auth\"]}"));
  // Los contadores reflejan todos los intentos aunque haya alertas suprimidas.
  const Metrics &m = r.cebo.metrics();
  CHECK(m.auth_attempts(Protocol::Http) == 5 && m.alerts() == 1 &&
        r.cebo.classifier().suppressed() == 3 && r.cebo.counters().published == 2);

  // El origen sigue en enfriamiento: más intentos se cuentan sin evento.
  for (unsigned n = 0; n < 3; ++n)
    CHECK(r.push(at(Protocol::Http, Kind::AuthAttempt, 2, 1800 + n * 10, Path::Login)));
  r.cebo.tick(1900);
  CHECK(r.ctx.events.size() == 2 && m.auth_attempts(Protocol::Http) == 8);
  // Telnet y otro origen HTTP: su propia entrada.
  CHECK(r.push(at(Protocol::Telnet, Kind::Input, 2, 2000)));
  CHECK(r.push(at(Protocol::Http, Kind::Access, 9, 2000, Path::Root)));
  r.cebo.tick(2100);
  CHECK(r.ctx.events.size() == 4 && m.inputs(Protocol::Telnet) == 1 &&
        m.accesses(Protocol::Http) == 1);
  CHECK(r.ctx.events[2].severity == "info" && has(r.ctx.events[2].details, "\"protocol\":\"telnet\""));

  // Un evento que el núcleo no acepta se reintenta y, pasado el plazo, se
  // descarta sin contarse como alerta ni iniciar enfriamiento; la siguiente
  // interacción del origen vuelve a publicarse.
  Rig d;
  CHECK(d.cebo.start());
  for (unsigned n = 0; n < 5; ++n)
    CHECK(d.push(at(Protocol::Http, Kind::AuthAttempt, 4, 1100 + n, Path::Login)));
  d.ctx.accept = false;
  d.cebo.tick(1200);                // la primera interacción espera al núcleo
  d.cebo.tick(1200 + EventRetryMs); // se descarta; la segunda queda esperando
  CHECK(d.cebo.counters().publish_failed == 1 && d.cebo.metrics().alerts() == 0 &&
        d.cebo.metrics().auth_attempts(Protocol::Http) == 2 && d.cebo.bus().size() == 3);
  d.ctx.accept = true;
  d.cebo.tick(1300 + EventRetryMs);
  CHECK(d.ctx.events.size() == 2 && d.cebo.bus().size() == 0 &&
        has(d.ctx.events[0].details, "\"count\":1,\"window_ms\":1000") &&
        d.ctx.events[1].severity == "warning" &&
        has(d.ctx.events[1].details,
            "\"count\":5,\"window_ms\":60000,\"suspicion\":\"medium\"") &&
        d.cebo.metrics().alerts() == 1 &&
        d.cebo.metrics().auth_attempts(Protocol::Http) == 5);
}

void metrics_and_stats() {
  Rig r;
  CHECK(r.cebo.start());
  r.p.lan = true;
  r.cebo.tick(1000);
  r.p.h.connections = 2;
  r.p.t.connections = 1;
  CHECK(r.push(at(Protocol::Http, Kind::Connect, 2, 1000)));
  CHECK(r.push(at(Protocol::Http, Kind::Access, 2, 1100, Path::Root)));
  CHECK(r.push(at(Protocol::Telnet, Kind::Connect, 3, 1200)));
  r.cebo.tick(1300);
  CHECK(r.ctx.metrics.empty());
  // Se rechaza una conexión por límite de capacidad.
  Limiter &l = r.cebo.limiter();
  r.cebo.limiter().configure(Config{1, 5000});
  CHECK(l.admit(Protocol::Http) && !l.admit(Protocol::Http));

  // Métricas periódicas v2 cada MetricsIntervalMs, con el tiempo real entre
  // publicaciones.
  r.cebo.tick(1000 + MetricsIntervalMs - 1);
  CHECK(r.ctx.metrics.empty());
  r.cebo.tick(1000 + MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 1 && r.ctx.metrics[0].interval_ms == MetricsIntervalMs);
  CHECK(r.ctx.metrics[0].values ==
        "{\"http\":{\"connections\":1,\"accesses\":1,\"auth_attempts\":0,"
        "\"inputs\":0,\"active_connections\":2,\"interactions_per_minute\":2,"
        "\"alerts\":0},\"telnet\":{\"connections\":1,\"accesses\":0,"
        "\"auth_attempts\":0,\"inputs\":0,\"active_connections\":1,"
        "\"interactions_per_minute\":1,\"alerts\":0},"
        "\"rejected_connections\":1,\"alerts\":0}");
  CHECK(r.cebo.counters().metrics_published == 1);
  // Una muestra que el núcleo rechaza se omite sin bloquear la siguiente.
  r.ctx.accept_metrics = false;
  r.cebo.tick(1000 + 2 * MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 1);
  r.ctx.accept_metrics = true;
  r.cebo.tick(1000 + 3 * MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 2 && r.ctx.metrics[1].interval_ms == MetricsIntervalMs);

  // cebo.stats: mismos valores, a demanda, sin alterar la cadencia periódica.
  r.ctx.now = 1000 + 3 * MetricsIntervalMs + 700;
  ModuleReply reply = r.command("cebo.stats", nullptr);
  CHECK(!reply.error && !reply.job);
  std::string text = reply.result;
  CHECK(text.rfind("{\"kind\":\"cebo.stats\",\"interval_ms\":700,\"values\":{\"http\":{", 0) == 0 &&
        has(text, "\"rejected_connections\":1,\"alerts\":0}}"));
  r.cebo.tick(1000 + 4 * MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 3 && r.ctx.metrics[2].interval_ms == MetricsIntervalMs);

  // Detener los servicios mantiene el modo activo y las métricas visibles.
  reply = r.command("cebo.stop", nullptr);
  CHECK(!reply.error);
  r.cebo.tick(1000 + 5 * MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 4);
  // Al salir del modo ya no se publica nada.
  r.cebo.request_stop();
  CHECK(r.cebo.stopped());
  r.cebo.tick(1000 + 7 * MetricsIntervalMs);
  CHECK(r.ctx.metrics.size() == 4);
  // Un modo nuevo empieza con contadores y límites a cero.
  CHECK(r.cebo.start());
  reply = r.command("cebo.stats", nullptr);
  CHECK(!reply.error && has(reply.result, "\"rejected_connections\":0,\"alerts\":0}}") &&
        has(reply.result, "\"connections\":0,\"accesses\":0"));
  CHECK(r.cebo.limiter().config() == Config() && r.cebo.limiter().active() == 0);
}

void privacy_of_outputs() {
  // La clasificación solo usa los metadatos de Interaction, que no puede
  // contener datos del cliente; aun así se comprueba que ninguna salida
  // contiene campos fuera del contrato cerrado.
  Rig r;
  CHECK(r.cebo.start());
  for (unsigned batch = 0; batch < 3; ++batch) {
    for (unsigned n = 0; n < 10; ++n)
      CHECK(r.push(at(Protocol::Telnet, n % 2 ? Kind::AuthAttempt : Kind::Input, 5,
                      1000 + batch * 100 + n * 10)));
    r.cebo.tick(1500 + batch * 10);
  }
  CHECK(!r.ctx.events.empty());
  for (const auto &e : r.ctx.events)
    for (const char *forbidden : {"password", "user", "pass", "cookie", "body",
                                  "query", "token", "authorization"})
      CHECK(!has(e.details, forbidden));
  ModuleReply stats = r.command("cebo.stats", nullptr);
  for (const char *forbidden : {"password", "cookie", "body", "token"})
    CHECK(!has(stats.result, forbidden));
}
} // namespace

void events_tests() {
  classifier_isolated();
  classifier_repeated_auth();
  classifier_rate_threshold();
  classifier_connection_flood();
  classifier_windows_and_expiry();
  classifier_capacity();
  classifier_unconfirmed();
  std::cout << "Cebo classifier (isolated, repeated, rate, windows, expiry, LRU, cooldown): PASS\n";
  metrics_values();
  metrics_rate_window();
  std::cout << "Cebo metrics (counters survive suppression, rolling rate, v2 shape): PASS\n";
  limits();
  std::cout << "Cebo shared limits (max_connections, per-service caps, rejections): PASS\n";
  store_persistence();
  std::cout << "Cebo config store (commit-before-ack, corruption, failures, privacy): PASS\n";
  configure_command();
  restore_command();
  std::cout << "Cebo cebo.configure and restore (validated, committed, not auto-started): PASS\n";
  events_flow();
  metrics_and_stats();
  privacy_of_outputs();
  std::cout << "Cebo events, metrics and cebo.stats through the module hooks: PASS\n";
}

// Muestras reales del código para el decodificador v2 compartido
// (check_contracts.mjs): una línea por muestra, con etiqueta y JSON.
void events_dump() {
  Rig r;
  CHECK(r.cebo.start());
  r.p.lan = true;
  r.cebo.tick(1000);
  r.p.h.connections = 3;
  r.p.t.connections = 1;
  uint64_t t = 1000, tick_now = 1100;
  auto push = [&](Protocol p, Kind k, uint8_t host, Path path = Path::None) {
    Interaction i = at(p, k, host, t, path);
    for (unsigned attempts = 0; !r.push(i); ++attempts) {
      CHECK(attempts < 8); // el bus se vacía con los ticks del módulo
      r.cebo.tick(++tick_now);
    }
    t += 50;
  };
  // Informativo HTTP, repetición de login (medio), tasa alta Telnet y barrido
  // de conexiones.
  push(Protocol::Http, Kind::Access, 2, Path::Root);
  for (unsigned n = 0; n < 5; ++n)
    push(Protocol::Http, Kind::AuthAttempt, 3, Path::Login);
  push(Protocol::Telnet, Kind::Connect, 4);
  for (unsigned n = 0; n < 12; ++n) {
    push(Protocol::Telnet, Kind::Input, 4);
    push(Protocol::Telnet, Kind::AuthAttempt, 4);
  }
  for (unsigned n = 0; n < 20; ++n)
    push(Protocol::Http, Kind::Connect, 6);
  for (unsigned round = 0; round < 8; ++round)
    r.cebo.tick(++tick_now);
  for (const auto &e : r.ctx.events)
    std::cout << "event " << e.severity << ' ' << e.details << '\n';
  r.cebo.limiter().admit(Protocol::Http); // activa un rechazo real
  r.cebo.limiter().configure(Config{1, 5000});
  r.cebo.limiter().admit(Protocol::Http);
  r.cebo.tick(1000 + MetricsIntervalMs);
  CHECK(!r.ctx.metrics.empty());
  std::cout << "metrics " << r.ctx.metrics.back().values << '\n';
  r.ctx.now = 1000 + MetricsIntervalMs + 5;
  ModuleReply stats = r.command("cebo.stats", nullptr);
  CHECK(!stats.error);
  std::cout << "stats " << stats.result << '\n';
  // Estado tras cebo.configure.
  Args args(true, true, 2, 15000);
  ModuleReply reply = r.command("cebo.configure", &args.root);
  CHECK(!reply.error);
  std::cout << "module " << state_of(r.cebo) << '\n';
  // Diagnóstico por registro dañado.
  FakeCebo broken;
  broken.store.load_result = Load::Error;
  Cebo c(broken);
  CHECK(c.init(r.ctx) && c.start());
  r.ctx.events.clear();
  c.tick(2000);
  CHECK(r.ctx.events.size() == 1);
  std::cout << "status " << r.ctx.events[0].details << '\n';
}
