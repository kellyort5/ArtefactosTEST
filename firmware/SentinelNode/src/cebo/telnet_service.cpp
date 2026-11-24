// Adaptador ESP-IDF del Telnet simulado: un socket de escucha lwIP y una tarea
// que atiende hasta TelnetMaxSessions sesiones con select(). Toda la lógica de
// protocolo y privacidad está en telnet_session.cpp; aquí solo hay sockets.
#include "include/cebo.hpp"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <unistd.h>

namespace sentinel::cebo {
namespace {
uint64_t clock_ms() { return uint64_t(esp_timer_get_time()) / 1000; }

// Con tiempo de espera de select de 100 ms, la parada se completa en ese orden
// de magnitud, muy por debajo del plazo de 5 s del supervisor.
constexpr int SelectMicroseconds = 100000;

struct Slot {
  int fd = -1;
  TelnetSession session;
};

class EspTelnet final : public ServiceListener {
public:
  bool start(InteractionBus &bus, const ServiceSettings &settings) override {
    if (running_.load() || listen_fd_ >= 0)
      return false;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (fd < 0)
      return false;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(TelnetPort);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) !=
            0 ||
        listen(fd, 2) != 0) {
      close(fd);
      return false;
    }
    bus_ = &bus;
    idle_ms_ = settings.idle_timeout_ms;
    listen_fd_ = fd;
    stop_ = false;
    active_ = 0;
    running_ = true;
    if (xTaskCreate(&EspTelnet::run, "cebo_telnet", 4096, this,
                    tskIDLE_PRIORITY + 3, nullptr) != pdPASS) {
      running_ = false;
      listen_fd_ = -1;
      close(fd);
      return false;
    }
    return true;
  }

  // No bloquea: la tarea ve la marca en su siguiente vuelta, cierra todas las
  // sesiones y el listener, y solo entonces stopped() pasa a verdadero.
  void request_stop() override {
    if (running_.load())
      stop_ = true;
  }

  bool stopped() const override { return !running_.load(); }
  unsigned active_connections() const override { return active_.load(); }
  void tick() override {}

private:
  static void run(void *self) {
    static_cast<EspTelnet *>(self)->loop();
    vTaskDelete(nullptr);
  }

  // Solo IPv4: el contrato v2 no tiene otra representación de origen.
  static bool peer_of(const sockaddr_in &address, Peer &out) {
    if (address.sin_family != AF_INET || address.sin_port == 0)
      return false;
    memcpy(out.ipv4, &address.sin_addr.s_addr, sizeof(out.ipv4));
    out.port = ntohs(address.sin_port);
    return true;
  }

  // Envío sin esperar: si el cliente no lee, la sesión se cierra en lugar de
  // bloquear la tarea o acumular salida.
  static bool flush(Slot &slot) {
    TelnetSession &s = slot.session;
    size_t length = s.output_length();
    if (!length)
      return true;
    int sent = send(slot.fd, s.output(), length, MSG_DONTWAIT);
    s.clear_output();
    return sent >= 0 && size_t(sent) == length;
  }

  void close_slot(Slot &slot, Close reason) {
    slot.session.finish(reason, clock_ms());
    close(slot.fd);
    slot = Slot();
  }

  void accept_one(Slot *slots) {
    sockaddr_in address = {};
    socklen_t length = sizeof(address);
    int fd = accept(listen_fd_, reinterpret_cast<sockaddr *>(&address), &length);
    if (fd < 0)
      return;
    Peer peer;
    Slot *free_slot = nullptr;
    for (unsigned n = 0; n < TelnetMaxSessions && !free_slot; ++n)
      if (slots[n].fd < 0)
        free_slot = &slots[n];
    if (!free_slot || !peer_of(address, peer)) {
      close(fd); // el exceso de sesiones se rechaza
      return;
    }
    free_slot->fd = fd;
    free_slot->session.open(peer, clock_ms(), *bus_);
    if (!flush(*free_slot))
      close_slot(*free_slot, Close::Peer);
  }

  void read_one(Slot &slot) {
    uint8_t buffer[128];
    int n = recv(slot.fd, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
      wipe(buffer, sizeof(buffer));
      close_slot(slot, Close::Peer);
      return;
    }
    if (n > 0) {
      bool keep = slot.session.feed(buffer, size_t(n), clock_ms());
      // Lo recibido solo existe en este buffer transitorio.
      wipe(buffer, sizeof(buffer));
      if (!flush(slot) || !keep)
        close_slot(slot, keep ? Close::Peer : slot.session.close_reason());
    }
  }

  void loop() {
    Slot slots[TelnetMaxSessions];
    while (!stop_.load()) {
      fd_set readable;
      FD_ZERO(&readable);
      FD_SET(listen_fd_, &readable);
      int highest = listen_fd_;
      for (const auto &s : slots)
        if (s.fd >= 0) {
          FD_SET(s.fd, &readable);
          if (s.fd > highest)
            highest = s.fd;
        }
      timeval wait = {0, SelectMicroseconds};
      int ready = select(highest + 1, &readable, nullptr, nullptr, &wait);
      if (ready < 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
      if (ready > 0) {
        if (FD_ISSET(listen_fd_, &readable))
          accept_one(slots);
        for (auto &s : slots)
          if (s.fd >= 0 && FD_ISSET(s.fd, &readable))
            read_one(s);
      }
      uint64_t now = clock_ms();
      unsigned open_sessions = 0;
      for (auto &s : slots) {
        if (s.fd >= 0 && s.session.idle(now, idle_ms_))
          close_slot(s, Close::IdleTimeout);
        open_sessions += s.fd >= 0;
      }
      active_ = open_sessions;
    }
    // Parada: se cierran las sesiones y el listener antes de anunciar el cierre.
    for (auto &s : slots)
      if (s.fd >= 0)
        close_slot(s, Close::Peer);
    close(listen_fd_);
    listen_fd_ = -1;
    active_ = 0;
    running_ = false;
  }

  InteractionBus *bus_ = nullptr;
  uint32_t idle_ms_ = IdleTimeoutMs;
  int listen_fd_ = -1;
  std::atomic<bool> running_{false}, stop_{false};
  std::atomic<unsigned> active_{0};
};
} // namespace

ServiceListener &esp_telnet_listener() {
  static EspTelnet listener;
  return listener;
}

} // namespace sentinel::cebo
