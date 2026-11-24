// Adaptador ESP-IDF del panel HTTP ficticio. Usa esp_http_server oficial: una
// sola tarea del servidor, sin tareas ni sockets propios. La política de
// respuestas y de privacidad está en http_policy.cpp.
#include "include/cebo.hpp"
#include <atomic>
#include <cstring>
#include <esp_http_server.h>
#include <esp_timer.h>
#include <lwip/sockets.h>
#include <mutex>
#include <unistd.h>

namespace sentinel::cebo {
namespace {
uint64_t clock_ms() { return uint64_t(esp_timer_get_time()) / 1000; }

// Sesiones rastreadas: IPv4 y puerto del origen, y última actividad para
// cerrar las que no completan peticiones. Margen de dos entradas por si el
// cierre de una sesión llega después de aceptar otra.
constexpr size_t SessionSlots = HttpMaxSockets + 2;
struct Session {
  int fd = -1;
  Peer peer;
  uint64_t last_ms = 0;
  bool closing = false;
};

// Solo se admiten orígenes IPv4 (directos o IPv4 mapeado en IPv6), porque el
// contrato v2 no tiene otra representación de origen.
bool peer_of_socket(int fd, Peer &out) {
  sockaddr_storage address = {};
  socklen_t length = sizeof(address);
  if (getpeername(fd, reinterpret_cast<sockaddr *>(&address), &length) != 0)
    return false;
  if (address.ss_family == AF_INET) {
    const auto *a = reinterpret_cast<const sockaddr_in *>(&address);
    memcpy(out.ipv4, &a->sin_addr.s_addr, sizeof(out.ipv4));
    out.port = ntohs(a->sin_port);
    return out.port != 0;
  }
  if (address.ss_family == AF_INET6) {
    const auto *a = reinterpret_cast<const sockaddr_in6 *>(&address);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&a->sin6_addr);
    static const uint8_t mapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
    if (memcmp(bytes, mapped, sizeof(mapped)) != 0)
      return false;
    memcpy(out.ipv4, bytes + 12, sizeof(out.ipv4));
    out.port = ntohs(a->sin6_port);
    return out.port != 0;
  }
  return false;
}

// El servidor tiene un solo hilo: el vaciado del cuerpo tiene un presupuesto
// total para que un cliente lento no lo ocupe indefinidamente.
class RequestBody final : public BodyReader {
public:
  explicit RequestBody(httpd_req_t *request)
      : request_(request), started_(clock_ms()) {}
  int read(uint8_t *buffer, size_t capacity) override {
    if (clock_ms() - started_ > HttpBodyBudgetMs)
      return -1;
    return httpd_req_recv(request_, reinterpret_cast<char *>(buffer),
                          capacity);
  }

private:
  httpd_req_t *request_;
  uint64_t started_;
};

class EspHttp final : public ServiceListener {
public:
  bool start(InteractionBus &bus, const ServiceSettings &settings) override {
    if (server_)
      return false;
    if (!settings.limiter)
      return false;
    bus_ = &bus;
    limiter_ = settings.limiter;
    limiter_->reset(Protocol::Http);
    {
      std::lock_guard<std::mutex> guard(lock_);
      for (auto &s : sessions_)
        s = Session();
    }
    active_ = 0;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HttpPort;
    // Un socket más que los admitidos: el excedente llega a on_open, donde el
    // límite lo rechaza y lo cuenta, en lugar de descartarlo el servidor sin
    // que el módulo lo vea.
    config.max_open_sockets = HttpMaxSockets + 1;
    config.max_uri_handlers = 2;
    config.max_resp_headers = 8;
    config.backlog_conn = 2;
    config.lru_purge_enable = false; // el exceso se rechaza, no desplaza
    config.recv_wait_timeout = HttpTimeoutSeconds;
    config.send_wait_timeout = HttpTimeoutSeconds;
    config.stack_size = 6144;
    config.global_user_ctx = this;
    config.open_fn = &EspHttp::on_open;
    config.close_fn = &EspHttp::on_close;
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t handle = nullptr;
    if (httpd_start(&handle, &config) != ESP_OK)
      return false;
    // Un único manejador para cualquier ruta y método: la clasificación y las
    // respuestas están en handle_request.
    httpd_uri_t any = {};
    any.uri = "/*";
    any.method = static_cast<httpd_method_t>(HTTP_ANY); // macro entera INT_MAX
    any.handler = &EspHttp::on_request;
    any.user_ctx = this;
    if (httpd_register_uri_handler(handle, &any) != ESP_OK) {
      httpd_stop(handle);
      return false;
    }
    server_ = handle;
    return true;
  }

  void request_stop() override {
    if (!server_)
      return;
    // httpd_stop cierra todas las sesiones y el listener. Si falla, el
    // servidor se conserva y el siguiente intento ocurre en el siguiente tick.
    if (httpd_stop(server_) != ESP_OK)
      return;
    server_ = nullptr;
    std::lock_guard<std::mutex> guard(lock_);
    for (auto &s : sessions_)
      s = Session();
    active_ = 0;
    if (limiter_)
      limiter_->reset(Protocol::Http);
  }

  bool stopped() const override { return server_ == nullptr; }
  unsigned active_connections() const override { return active_.load(); }

  void tick() override {
    if (!server_)
      return;
    int victims[SessionSlots];
    size_t count = 0;
    uint64_t now = clock_ms();
    const uint32_t idle_ms = limiter_ ? limiter_->idle_timeout_ms() : IdleTimeoutMs;
    {
      std::lock_guard<std::mutex> guard(lock_);
      for (auto &s : sessions_)
        if (s.fd >= 0 && !s.closing && now - s.last_ms >= idle_ms) {
          s.closing = true;
          victims[count++] = s.fd;
        }
    }
    for (size_t i = 0; i < count; ++i)
      if (httpd_sess_trigger_close(server_, victims[i]) != ESP_OK) {
        std::lock_guard<std::mutex> guard(lock_);
        for (auto &s : sessions_)
          if (s.fd == victims[i])
            s.closing = false;
      }
  }

private:
  static esp_err_t on_open(httpd_handle_t handle, int fd) {
    auto *self = static_cast<EspHttp *>(httpd_get_global_user_ctx(handle));
    Peer peer;
    if (!self || !self->limiter_ || !self->limiter_->admit(Protocol::Http))
      return ESP_FAIL; // sin cupo: el servidor cierra el socket de inmediato
    if (!peer_of_socket(fd, peer) || !self->track(fd, peer)) {
      self->limiter_->release(Protocol::Http);
      return ESP_FAIL;
    }
    self->note(Kind::Connect, peer, Close::None);
    return ESP_OK;
  }

  // Con close_fn propio, el servidor no cierra el socket: se hace aquí. El
  // descriptor puede ya no ser válido.
  static void on_close(httpd_handle_t handle, int fd) {
    auto *self = static_cast<EspHttp *>(httpd_get_global_user_ctx(handle));
    Session session;
    if (self && self->untrack(fd, session)) {
      if (self->limiter_)
        self->limiter_->release(Protocol::Http);
      self->note(Kind::Disconnect, session.peer,
                 session.closing ? Close::IdleTimeout : Close::Peer);
    }
    close(fd);
  }

  static esp_err_t on_request(httpd_req_t *request) {
    auto *self = static_cast<EspHttp *>(request->user_ctx);
    int fd = httpd_req_to_sockfd(request);
    Peer peer;
    if (!self || fd < 0 || !self->lookup(fd, peer))
      return ESP_FAIL; // sin origen conocido: se cierra sin responder
    Method method = request->method == HTTP_GET    ? Method::Get
                    : request->method == HTTP_HEAD ? Method::Head
                    : request->method == HTTP_POST ? Method::Post
                                                   : Method::Other;
    self->touch(fd);
    RequestBody body(request);
    Response response = handle_request(method, request->uri,
                                       request->content_len, body, peer,
                                       clock_ms(), *self->bus_);
    httpd_resp_set_status(request, response.status);
    httpd_resp_set_type(request, response.content_type);
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(request, "Content-Security-Policy",
                       "default-src 'none'; form-action 'self'");
    if (response.allow)
      httpd_resp_set_hdr(request, "Allow", response.allow);
    if (response.close)
      httpd_resp_set_hdr(request, "Connection", "close");
    bool head = method == Method::Head;
    esp_err_t sent = httpd_resp_send(request, head ? nullptr : response.body,
                                     head ? 0 : HTTPD_RESP_USE_STRLEN);
    self->touch(fd);
    if (response.close || sent != ESP_OK)
      httpd_sess_trigger_close(request->handle, fd);
    return sent;
  }

  bool track(int fd, const Peer &peer) {
    std::lock_guard<std::mutex> guard(lock_);
    Session *slot = nullptr;
    for (auto &s : sessions_)
      if (s.fd == fd) { // descriptor reutilizado sin cierre observado
        slot = &s;
        break;
      }
    if (!slot)
      for (auto &s : sessions_)
        if (s.fd < 0) {
          slot = &s;
          break;
        }
    if (!slot)
      return false;
    *slot = Session();
    slot->fd = fd;
    slot->peer = peer;
    slot->last_ms = clock_ms();
    recount();
    return true;
  }

  bool untrack(int fd, Session &out) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto &s : sessions_)
      if (s.fd == fd) {
        out = s;
        s = Session();
        recount();
        return true;
      }
    return false;
  }

  bool lookup(int fd, Peer &out) {
    std::lock_guard<std::mutex> guard(lock_);
    for (const auto &s : sessions_)
      if (s.fd == fd) {
        out = s.peer;
        return true;
      }
    return false;
  }

  void touch(int fd) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto &s : sessions_)
      if (s.fd == fd)
        s.last_ms = clock_ms();
  }

  void recount() { // con lock_ tomado
    unsigned n = 0;
    for (const auto &s : sessions_)
      n += s.fd >= 0;
    active_ = n;
  }

  void note(Kind kind, const Peer &peer, Close close_reason) {
    if (!bus_)
      return;
    Interaction i;
    memcpy(i.ipv4, peer.ipv4, sizeof(i.ipv4));
    i.source_port = peer.port;
    i.destination_port = HttpPort;
    i.protocol = Protocol::Http;
    i.kind = kind;
    i.close = close_reason;
    i.uptime_ms = clock_ms();
    bus_->push(i);
  }

  httpd_handle_t server_ = nullptr;
  InteractionBus *bus_ = nullptr;
  Limiter *limiter_ = nullptr;
  std::mutex lock_;
  Session sessions_[SessionSlots];
  std::atomic<unsigned> active_{0};
};
} // namespace

ServiceListener &esp_http_listener() {
  static EspHttp listener;
  return listener;
}

} // namespace sentinel::cebo
