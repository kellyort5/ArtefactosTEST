#include "include/mqtt_dns.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include <Arduino.h>
#include <cstring>
#include <new>
namespace sentinel {
struct MqttDns::Query {
  MqttDns *owner;
  uint32_t generation;
  char host[257];
};
void MqttDns::complete(const char *, const ip_addr_t *ip, void *arg) {
  auto q = static_cast<Query *>(arg);
  auto &s = *q->owner;
  portENTER_CRITICAL(&s.mux);
  if (q->generation == s.generation) {
    s.valid = ip != nullptr;
    if (ip)
      s.address = *ip;
    s.ready = true;
  }
  --s.pending;
  portEXIT_CRITICAL(&s.mux);
  delete q;
}
void MqttDns::begin(void *arg) {
  auto q = static_cast<Query *>(arg);
  ip_addr_t ip = {};
  err_t rc = dns_gethostbyname_addrtype(q->host, &ip, complete, q,
                                        LWIP_DNS_ADDRTYPE_IPV4_IPV6);
  if (rc == ERR_OK)
    complete(nullptr, &ip, q);
  else if (rc != ERR_INPROGRESS)
    complete(nullptr, nullptr, q);
}
bool MqttDns::resolve(const char *host, IPAddress &out,
                      const std::atomic<bool> &cancel) {
  if (out.fromString(host))
    return !cancel.load();
  auto q = new (std::nothrow) Query;
  if (!q)
    return false;
  portENTER_CRITICAL(&mux);
  if (pending >= 2) {
    portEXIT_CRITICAL(&mux);
    delete q;
    return false;
  }
  ++pending;
  q->generation = ++generation;
  ready = valid = false;
  portEXIT_CRITICAL(&mux);
  q->owner = this;
  snprintf(q->host, sizeof(q->host), "%s", host);
  if (tcpip_try_callback(begin, q) != ERR_OK) {
    complete(nullptr, nullptr, q);
    return false;
  }
  uint32_t deadline = millis() + 1000;
  while (!cancel && static_cast<int32_t>(deadline - millis()) > 0) {
    portENTER_CRITICAL(&mux);
    bool done = ready, ok = valid;
    ip_addr_t ip = address;
    portEXIT_CRITICAL(&mux);
    if (done) {
      char text[48];
      return ok && ipaddr_ntoa_r(&ip, text, sizeof(text)) &&
             out.fromString(text);
    }
    delay(1);
  }
  // Invalidate the callback without freeing its context before lwIP returns it.
  portENTER_CRITICAL(&mux);
  ++generation;
  portEXIT_CRITICAL(&mux);
  return false;
}
} // namespace sentinel
