#pragma once
// Cebo: servicios señuelo HTTP (sentinel-cebo-http) y Telnet
// (sentinel-cebo-telnet). Este encabezado separa la lógica pura (bus, política
// HTTP, sesión Telnet, ciclo de vida) del pegamento ESP-IDF, de modo que se
// pueda comprobar en host sin radio ni sockets reales.
#include "../../sentinel_api/include/module_api.hpp"
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace sentinel::cebo {

// Límites iniciales: propuestas de diseño ajustables ante fallos; no son
// mediciones de capacidad.
constexpr uint16_t HttpPort = 80;
constexpr unsigned HttpMaxSockets = 4;       // el exceso de conexiones se rechaza
constexpr unsigned HttpTimeoutSeconds = 5;   // recv/send del servidor
constexpr size_t HttpMaxBody = 1024;         // cuerpo procesable
constexpr uint32_t HttpBodyBudgetMs = 3000;  // tiempo total para vaciar un cuerpo
constexpr uint16_t TelnetPort = 23;
constexpr unsigned TelnetMaxSessions = 2;
constexpr size_t TelnetMaxLine = 512;          // bytes de datos por línea
constexpr size_t TelnetMaxSessionBytes = 4096; // bytes recibidos por sesión
constexpr unsigned TelnetMaxAttempts = 3;      // intentos antes de cerrar
// Total de conexiones de los dos servicios: coincide con el máximo del contrato.
constexpr unsigned MaxConnections = HttpMaxSockets + TelnetMaxSessions;
static_assert(MaxConnections == 6, "contract max_connections is 6");
constexpr uint32_t IdleTimeoutMs = 5000; // sesión sin actividad, en ambos servicios
constexpr uint32_t ServiceRetryMs = 5000; // espera tras un fallo al arrancar
constexpr size_t BusCapacity = 16;
constexpr unsigned EventsPerTick = 4;
constexpr uint32_t EventRetryMs = 2000; // evento no aceptado: se descarta tras esto
constexpr uint32_t IsolatedWindowMs = 1000;
// Rango de idle_timeout_ms que acepta el contrato v2 (cebo.configure).
constexpr uint32_t MinIdleTimeoutMs = 1000, MaxIdleTimeoutMs = 60000;
constexpr uint32_t MetricsIntervalMs = 5000; // periodo de las métricas v2
// Clasificación por origen y protocolo (sentinel-cebo-eventos-historial).
// Son propuestas de diseño ajustables ante fallos; indicios de conducta, no
// prueba de intrusión.
constexpr size_t ClassifierEntries = 32;  // tabla con desalojo LRU
constexpr uint32_t WindowMs = 60000;      // ventana de conteo
constexpr uint32_t ExpiryMs = 300000;     // una entrada sin actividad caduca
constexpr uint32_t CooldownMs = 30000;    // entre eventos del mismo origen
constexpr uint32_t MediumAuthAttempts = 5;   // intentos de login/ventana: medio
constexpr uint32_t HighInteractions = 20;    // interacciones/ventana: alto
// El núcleo solo admite cebo.start cuando existen HTTP y Telnet
// (module_runtime.cpp). Mientras la app no tenga el control de Cebo, los
// servicios arrancan al entrar en el modo, que ya es una orden explícita del
// usuario; cebo.start/cebo.stop siguen funcionando una vez dentro.
constexpr bool AutoStartServices = true;

enum class Protocol : uint8_t { Http, Telnet };
enum class Kind : uint8_t { Connect, Access, AuthAttempt, Input, Disconnect };
// Rutas permitidas por el contrato v2; cualquier otra se clasifica como Other.
enum class Path : uint8_t { None, Root, Login, Admin, Favicon, Robots, Other };
// Motivo de un cierre: el origen cerró, tiempo de inactividad o un límite
// propio (línea, bytes o intentos).
enum class Close : uint8_t { None, Peer, IdleTimeout, Limit };

// Metadatos normalizados de una interacción. No existe ningún campo capaz de
// contener usuario, contraseña, cookie, cabecera, consulta, línea ni cuerpo.
struct Interaction {
  uint8_t ipv4[4] = {};
  uint16_t source_port = 0, destination_port = 0;
  Protocol protocol = Protocol::Http;
  Kind kind = Kind::Connect;
  Path path = Path::None;
  Close close = Close::None;
  uint64_t uptime_ms = 0;
};

const char *protocol_text(Protocol);
const char *kind_text(Kind);
const char *path_text(Path); // nullptr para Path::None
// Código de evidencia v2 propio de la interacción (V2CeboEvidence).
const char *evidence_text(const Interaction &);

// Nivel de sospecha de una interacción: indicio, nunca atribución. Se liga a
// la severidad del contrato: informational→info, medium→warning, high→critical.
enum class Suspicion : uint8_t { Informational, Medium, High };
const char *suspicion_text(Suspicion);
const char *severity_text(Suspicion);

// Resultado de clasificar una interacción: si debe publicarse un evento y con
// qué sospecha, cantidad, ventana y evidencia. No contiene datos del cliente.
struct Classification {
  bool publish = false;
  Suspicion suspicion = Suspicion::Informational;
  uint32_t count = 1, window_ms = IsolatedWindowMs;
  // Evidencia propia de la interacción y, si procede, la de repetición.
  const char *evidence[2] = {nullptr, nullptr};
  unsigned evidence_count = 0;
  // Posición de la tabla para confirmar la publicación (Classifier::commit).
  unsigned slot = 0;
};

// Detalles v2 exactos de `cebo.interaction` con su clasificación. Falso si el
// resultado no cabe o los datos no cumplen el contrato.
bool format_details(const Interaction &, const Classification &, char *out,
                    size_t capacity);
// Atajo: interacción aislada (count 1, informational).
bool format_details(const Interaction &, char *out, size_t capacity);
// Se publican accesos, entradas e intentos de autenticación; una conexión
// solo si escala la sospecha del origen. El cierre nunca genera evento.
bool publishable(const Interaction &);
// Borrado que el compilador no puede eliminar, para buffers con datos del
// cliente.
void wipe(void *data, size_t length);

// Cola acotada de interacciones. Varios productores (tareas de los servicios)
// y un único consumidor (supervisor). Al llenarse se pierde la más reciente y
// se contabiliza; nunca bloquea ni crece.
class InteractionBus {
public:
  bool push(const Interaction &);
  bool front(Interaction &out) const;
  void pop();
  void open();  // vacía y acepta nuevas interacciones
  void close(); // vacía y rechaza hasta open()
  size_t size() const;
  uint32_t dropped() const;

private:
  mutable std::mutex lock_;
  Interaction items_[BusCapacity];
  size_t head_ = 0, count_ = 0;
  uint32_t dropped_ = 0;
  bool closed_ = false;
};

struct Peer {
  uint8_t ipv4[4] = {};
  uint16_t port = 0;
};

// ---- Configuración y límites ----------------------------------------------
// Configuración aceptada de Cebo (cebo.configure). El contrato solo admite
// HTTP y Telnet activos, de modo que solo estos dos valores son variables.
struct Config {
  unsigned max_connections = MaxConnections; // simultáneas, ambos servicios
  uint32_t idle_timeout_ms = IdleTimeoutMs;
  bool operator==(const Config &o) const {
    return max_connections == o.max_connections &&
           idle_timeout_ms == o.idle_timeout_ms;
  }
};
bool valid(const Config &);

enum class Load : uint8_t { Loaded, Missing, Error };
// Almacén de la configuración aceptada. save solo devuelve verdadero tras el
// commit; ante un fallo conserva la configuración anterior.
class ConfigStore {
public:
  virtual ~ConfigStore() = default;
  virtual Load load(Config &out) = 0;
  virtual bool save(const Config &) = 0;
};
// NVS estándar, espacio de nombres propio de Cebo (config_store.cpp).
ConfigStore &nvs_config_store();

// Admisión de conexiones compartida por HTTP y Telnet. Hace cumplir
// max_connections (total) y el tope propio de cada servicio, y cuenta las
// conexiones rechazadas. Se usa desde las tareas de los servicios.
class Limiter {
public:
  void configure(const Config &);
  Config config() const;
  uint32_t idle_timeout_ms() const;
  // Reserva una conexión; falso (y cuenta un rechazo) si no cabe.
  bool admit(Protocol);
  void release(Protocol);
  // Pone a cero las conexiones de un servicio cuando se cierran todas.
  void reset(Protocol);
  // Nueva sesión del modo: sin conexiones ni rechazos.
  void clear();
  unsigned active() const;
  unsigned active(Protocol) const;
  uint32_t rejected() const;

private:
  mutable std::mutex lock_;
  Config config_;
  unsigned active_[2] = {};
  uint32_t rejected_ = 0;
};

// ---- Clasificación ----------------------------------------------------------
// Tabla de orígenes por (IPv4, protocolo): ventanas de WindowMs, expiración,
// desalojo LRU y enfriamiento entre eventos. Cuenta todas las interacciones
// aunque el evento se suprima. Lógica pura: el reloj lo da la interacción.
//
//  - isolated: acceso/entrada/login sin repetición suficiente → informational.
//  - ≥ MediumAuthAttempts intentos de login en la ventana → medium
//    (repeated_auth); count = intentos.
//  - ≥ HighInteractions interacciones en la ventana → high (rate_threshold);
//    count = interacciones (conexión, acceso, entrada, login).
// Una escalada de nivel se publica aunque el origen esté en enfriamiento.
class Classifier {
public:
  // Cuenta la interacción y decide si publica. No modifica el estado de
  // publicación: eso lo hace commit tras confirmar que el evento se aceptó.
  Classification observe(const Interaction &);
  void commit(const Interaction &, const Classification &, bool accepted);
  // Elimina los orígenes sin actividad durante ExpiryMs.
  void expire(uint64_t now_ms);
  void reset();
  size_t size() const;
  uint32_t suppressed() const { return suppressed_; }
  uint32_t evictions() const { return evictions_; }

private:
  struct Entry {
    bool used = false, published = false;
    uint8_t ipv4[4] = {};
    Protocol protocol = Protocol::Http;
    uint8_t alerted = 0; // 0 ninguna, 1 medium, 2 high publicada en la ventana
    uint64_t window_start = 0, last_seen = 0, last_published = 0;
    uint32_t connections = 0, accesses = 0, auth_attempts = 0, inputs = 0;
  };
  Entry *find(const Interaction &);
  Entry *allocate(const Interaction &);
  Entry table_[ClassifierEntries];
  uint32_t suppressed_ = 0, evictions_ = 0;
};

// ---- Métricas ---------------------------------------------------------------
// Contadores acumulados del modo (conexiones, accesos, intentos, entradas,
// alertas) por protocolo, y ritmo de interacciones en los últimos 60 s.
class Metrics {
public:
  void record(const Interaction &);
  // Una alerta (medium/high) aceptada por el núcleo.
  void alert(Protocol);
  void reset();
  uint32_t alerts() const;
  // Valores v2 (V2CeboMetricValues). Falso si no caben.
  bool format(char *out, size_t capacity, unsigned http_active,
              unsigned telnet_active, uint32_t rejected,
              uint64_t now_ms) const;
  uint32_t connections(Protocol p) const { return side_[idx(p)].connections; }
  uint32_t accesses(Protocol p) const { return side_[idx(p)].accesses; }
  uint32_t auth_attempts(Protocol p) const { return side_[idx(p)].auth; }
  uint32_t inputs(Protocol p) const { return side_[idx(p)].inputs; }
  uint32_t per_minute(Protocol p, uint64_t now_ms) const;

private:
  static constexpr unsigned Buckets = 60;
  struct Side {
    uint32_t connections = 0, accesses = 0, auth = 0, inputs = 0, alerts = 0;
    uint64_t bucket_second[Buckets] = {};
    uint32_t bucket_count[Buckets] = {};
    bool bucket_used[Buckets] = {};
  };
  static unsigned idx(Protocol p) { return p == Protocol::Telnet ? 1 : 0; }
  Side side_[2];
};

// ---- HTTP -----------------------------------------------------------------
enum class Method : uint8_t { Get, Head, Post, Other };
// Origen del cuerpo de una petición. read devuelve bytes (>0), 0 al terminar o
// un valor negativo ante error o tiempo agotado.
class BodyReader {
public:
  virtual ~BodyReader() = default;
  virtual int read(uint8_t *buffer, size_t capacity) = 0;
};
// Respuesta fija: ninguna parte depende de datos recibidos.
struct Response {
  const char *status;
  const char *content_type;
  const char *body;
  const char *allow; // nullptr si no aplica
  bool close;
};
// Ruta canónica sin consulta ni fragmento; la URI original no se conserva.
Path canonical_path(const char *uri);
// Registra la interacción y decide la respuesta. El cuerpo de un formulario se
// vacía en un buffer transitorio que se borra de inmediato; nunca se interpreta.
Response handle_request(Method, const char *uri, size_t content_length,
                        BodyReader &, const Peer &, uint64_t now_ms,
                        InteractionBus &);

// ---- Telnet ---------------------------------------------------------------
// Sesión Telnet simulada: saludo, login, password y respuesta fija. Procesa
// bytes en cualquier fragmentación. De cada línea solo se cuenta su longitud:
// nunca se guarda, se refleja ni se interpreta, y nada se ejecuta. La
// negociación IAC es acotada: solo se rechazan opciones, sin ofrecer
// funciones salvo suprimir el eco durante la contraseña.
class TelnetSession {
public:
  // Registra la conexión y prepara el saludo y el primer prompt.
  void open(const Peer &, uint64_t now_ms, InteractionBus &);
  // Procesa bytes recibidos. Falso cuando la sesión debe cerrarse una vez
  // enviada la salida pendiente.
  bool feed(const uint8_t *data, size_t length, uint64_t now_ms);
  bool idle(uint64_t now_ms, uint32_t timeout_ms) const;
  // Cierra la sesión una sola vez y registra la desconexión.
  void finish(Close reason, uint64_t now_ms);
  const uint8_t *output() const { return out_; }
  size_t output_length() const { return out_length_; }
  void clear_output() { out_length_ = 0; }
  bool closing() const { return closing_; }
  bool finished() const { return finished_; }
  Close close_reason() const { return reason_; }

private:
  enum class Stage : uint8_t { User, Password };
  enum class Parser : uint8_t { Data, Iac, Option, Sub, SubIac };
  void step(uint8_t byte);
  void data_byte(uint8_t byte);
  void end_line();
  void negotiate(uint8_t command, uint8_t option);
  void emit(const uint8_t *bytes, size_t length);
  void record(Kind, Close = Close::None);
  void close_with(Close);
  Peer peer_;
  InteractionBus *bus_ = nullptr;
  uint64_t now_ = 0, last_ms_ = 0;
  uint8_t out_[256];
  size_t out_length_ = 0, line_ = 0, bytes_ = 0, sub_ = 0;
  unsigned attempts_ = 0, replies_ = 0;
  uint8_t command_ = 0;
  Stage stage_ = Stage::User;
  Parser parser_ = Parser::Data;
  Close reason_ = Close::None;
  bool after_cr_ = false, closing_ = false, finished_ = false;
};

// ---- Servicios y módulo ---------------------------------------------------
// Límites compartidos de los dos servicios. El límite vive en el módulo y los
// servicios lo consultan en cada conexión, de modo que cebo.configure surte
// efecto sin reiniciar listeners.
struct ServiceSettings {
  Limiter *limiter = nullptr;
};
// Un servicio de red (HTTP o Telnet). Las implementaciones ESP-IDF viven en
// http_service.cpp y telnet_service.cpp; las pruebas de host usan dobles.
class ServiceListener {
public:
  virtual ~ServiceListener() = default;
  // Abre el listener. Las interacciones se depositan en el bus.
  virtual bool start(InteractionBus &, const ServiceSettings &) = 0;
  // Inicia el cierre de todas las sesiones y del listener; idempotente y sin
  // bloquear.
  virtual void request_stop() = 0;
  // Verdadero solo cuando no queda listener, tarea ni socket.
  virtual bool stopped() const = 0;
  virtual unsigned active_connections() const = 0;
  // Mantenimiento periódico en el hilo del supervisor.
  virtual void tick() = 0;
};
class CeboPlatform {
public:
  virtual ~CeboPlatform() = default;
  // Hay IPv4 en la LAN autorizada gestionada por el núcleo.
  virtual bool lan_ready() = 0;
  virtual ServiceListener &http() = 0;
  virtual ServiceListener &telnet() = 0;
  virtual ConfigStore &config() = 0;
};
// Implementaciones ESP-IDF; no se usan en las pruebas de host.
ServiceListener &esp_http_listener();
ServiceListener &esp_telnet_listener();

enum class Service : uint8_t { Stopped, Starting, Running, Error };
const char *service_text(Service);

struct Counters {
  uint32_t interactions[5] = {}; // por Kind, consumidas del bus
  uint32_t published = 0, publish_failed = 0;
  uint32_t metrics_published = 0;
};

class Cebo final : public Module {
public:
  explicit Cebo(CeboPlatform &platform) : platform_(platform) {
    services_[0].listener = &platform.http();
    services_[1].listener = &platform.telnet();
  }
  bool init(ModuleContext &) override;
  bool start() override;
  void request_stop() override;
  bool stopped() const override;
  void handle(const ModuleCommand &, ModuleReply &) override;
  void tick(uint64_t now_ms) override;
  void snapshot(ModuleSnapshot &) const override;
  // Observación para pruebas.
  Service http_status() const { return services_[0].status; }
  Service telnet_status() const { return services_[1].status; }
  const Counters &counters() const { return counters_; }
  const Classifier &classifier() const { return classifier_; }
  const Metrics &metrics() const { return metrics_; }
  Limiter &limiter() { return limiter_; }
  Config config() const { return config_; }
  InteractionBus &bus() { return bus_; }

private:
  struct Control {
    ServiceListener *listener = nullptr;
    Service status = Service::Stopped;
    uint64_t retry_at = 0;
  };
  // Evento ya clasificado a la espera de que el núcleo lo acepte.
  struct Pending {
    bool used = false;
    Interaction interaction;
    Classification classification;
    char details[320] = {};
    uint64_t since = 0;
    bool failing = false;
  };
  void step(Control &, bool lan, uint64_t now_ms);
  void drain(uint64_t now_ms);
  bool deliver(uint64_t now_ms);
  void publish_metrics(uint64_t now_ms);
  void report_storage_error();
  void configure(const ModuleCommand &, ModuleReply &);
  void stats(ModuleReply &);
  void finish_stop();
  bool all_stopped() const;
  CeboPlatform &platform_;
  ModuleContext *context_ = nullptr;
  InteractionBus bus_;
  Limiter limiter_;
  Classifier classifier_;
  Metrics metrics_;
  Counters counters_;
  Config config_;
  Pending pending_;
  Control services_[2];
  bool active_ = false, stopping_ = false, wanted_ = false;
  bool storage_error_ = false;
  bool stored_ok_ = false; // lo almacenado coincide con config_
  uint64_t last_metrics_ = 0;
};

} // namespace sentinel::cebo
