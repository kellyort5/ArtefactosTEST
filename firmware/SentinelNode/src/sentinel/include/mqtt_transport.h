#pragma once
#include <cstddef>
#include <cstdint>
namespace sentinel {
// Owns MQTT I/O in one worker. Supervisor calls never wait for sockets.
class MqttTransport {
public:
  static constexpr size_t MaxPayload = 2048;
  struct Config {
    char host[257] = {}, user[257] = {}, password[513] = {};
    char client_id[72] = {}, commands[160] = {}, availability[160] = {};
    uint16_t port = 1883;
  };
  using StateCallback = void (*)(bool connected, bool error);
  using MessageCallback = void (*)(const char *, size_t, bool retained);
  bool init(StateCallback, MessageCallback);
  bool start(const Config &);
  void request_stop();
  bool stopped() const;
  bool stopping() const;
  bool connected() const;
  bool enqueue(const char *topic, const char *payload, size_t, int qos,
               bool retained);

private:
  struct Impl;
  Impl *impl = nullptr; // Process lifetime, like the central Network owner.
};
} // namespace sentinel
