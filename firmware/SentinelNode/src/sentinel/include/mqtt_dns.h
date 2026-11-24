#pragma once
#include "freertos/FreeRTOS.h"
#include "lwip/ip_addr.h"
#include <IPAddress.h>
#include <atomic>
namespace sentinel {
// Uses lwIP's resolver on its own thread, with bounded outstanding requests.
// Late callbacks cannot outlive this process-lifetime owner or a new
// generation.
class MqttDns {
public:
  bool resolve(const char *, IPAddress &, const std::atomic<bool> &cancel);

private:
  struct Query;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  uint32_t generation = 0;
  unsigned pending = 0;
  bool ready = false, valid = false;
  ip_addr_t address = {};
  static void begin(void *);
  static void complete(const char *, const ip_addr_t *, void *);
};
} // namespace sentinel
