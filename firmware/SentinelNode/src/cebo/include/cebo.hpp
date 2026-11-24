#pragma once
// Cebo: panel HTTP ficticio (sentinel-cebo-http). Este encabezado separa la
// lógica pura (bus, política HTTP, ciclo de vida) del pegamento ESP-IDF, de modo
// que se pueda comprobar en host sin radio ni sockets reales.
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
constexpr uint32_t HttpIdleTimeoutMs = 5000; // sesión sin peticiones completas
constexpr uint32_t HttpRetryMs = 5000;       // espera tras un fallo al arrancar
constexpr size_t BusCapacity = 16;
constexpr unsigned EventsPerTick = 4;
constexpr uint32_t EventRetryMs = 2000; // evento no aceptado: se descarta tras esto
constexpr uint32_t IsolatedWindowMs = 1000;
// El núcleo solo admite cebo.start cuando existen HTTP y Telnet
// (module_runtime.cpp). Mientras Telnet no exista, el servicio HTTP arranca al
// entrar en el modo Cebo, que ya es una orden explícita del usuario.
constexpr bool AutoStartHttp = true;

enum class Protocol : uint8_t { Http, Telnet };
enum class Kind : uint8_t { Connect, Access, AuthAttempt, Input, Disconnect };
// Rutas permitidas por el contrato v2; cualquier otra se clasifica como Other.
enum class Path : uint8_t { None, Root, Login, Admin, Favicon, Robots, Other };
enum class Close : uint8_t { None, Peer, IdleTimeout };

// Metadatos normalizados de una interacción. No existe ningún campo capaz de
// contener usuario, contraseña, cookie, cabecera, consulta ni cuerpo.
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
// Detalles v2 exactos de `cebo.interaction` para una interacción aislada.
// Falso si el resultado no cabe o los datos no cumplen el contrato.
bool format_details(const Interaction &, char *out, size_t capacity);
// Esta entrega publica accesos e intentos de autenticación. Conexión y cierre
// circulan por el bus para la clasificación del siguiente cambio.
bool publishable(const Interaction &);

// Cola acotada de interacciones. Varios productores (tareas del servidor) y un
// único consumidor (supervisor). Al llenarse se pierde la más reciente y se
// contabiliza; nunca bloquea ni crece.
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

enum class Method : uint8_t { Get, Head, Post, Other };
struct Peer {
  uint8_t ipv4[4] = {};
  uint16_t port = 0;
};
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

struct HttpSettings {
  uint32_t idle_timeout_ms = HttpIdleTimeoutMs;
};
// Servicio de red subyacente. La implementación ESP-IDF vive en
// http_service.cpp; las pruebas de host usan un doble.
class HttpPlatform {
public:
  virtual ~HttpPlatform() = default;
  // Hay IPv4 en la LAN autorizada gestionada por el núcleo.
  virtual bool lan_ready() = 0;
  // Abre el listener. Las interacciones se depositan en el bus.
  virtual bool start(InteractionBus &, const HttpSettings &) = 0;
  // Inicia el cierre de todas las sesiones y del listener; idempotente.
  virtual void request_stop() = 0;
  // Verdadero solo cuando no queda listener, tarea ni socket.
  virtual bool stopped() const = 0;
  virtual unsigned active_connections() const = 0;
  // Mantenimiento periódico (sesiones inactivas) en el hilo del supervisor.
  virtual void tick() = 0;
};

// Implementación ESP-IDF (http_service.cpp); no se usa en las pruebas de host.
HttpPlatform &esp_http_platform();

enum class Service : uint8_t { Stopped, Starting, Running, Error };
const char *service_text(Service);

struct Counters {
  uint32_t interactions[5] = {}; // por Kind, consumidas del bus
  uint32_t published = 0, publish_failed = 0;
};

class Cebo final : public Module {
public:
  explicit Cebo(HttpPlatform &platform) : http_(platform) {}
  bool init(ModuleContext &) override;
  bool start() override;
  void request_stop() override;
  bool stopped() const override;
  void handle(const ModuleCommand &, ModuleReply &) override;
  void tick(uint64_t now_ms) override;
  void snapshot(ModuleSnapshot &) const override;
  // Observación para pruebas.
  Service http_status() const { return status_; }
  const Counters &counters() const { return counters_; }
  InteractionBus &bus() { return bus_; }

private:
  void drain(uint64_t now_ms);
  void finish_stop();
  HttpPlatform &http_;
  ModuleContext *context_ = nullptr;
  InteractionBus bus_;
  Counters counters_;
  Service status_ = Service::Stopped;
  bool active_ = false, stopping_ = false, wanted_ = false;
  uint64_t retry_at_ = 0, head_failing_since_ = 0;
  bool head_failing_ = false;
};

} // namespace sentinel::cebo
