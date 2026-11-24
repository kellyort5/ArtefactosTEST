#include "include/cebo.hpp"
#include <cstdio>
#include <cstring>

namespace sentinel::cebo {

const char *protocol_text(Protocol p) {
  return p == Protocol::Telnet ? "telnet" : "http";
}

const char *kind_text(Kind k) {
  switch (k) {
  case Kind::Connect:
    return "connect";
  case Kind::Access:
    return "access";
  case Kind::AuthAttempt:
    return "auth_attempt";
  case Kind::Input:
    return "input";
  case Kind::Disconnect:
    return "disconnect";
  }
  return "connect";
}

const char *path_text(Path p) {
  switch (p) {
  case Path::Root:
    return "/";
  case Path::Login:
    return "/login";
  case Path::Admin:
    return "/admin";
  case Path::Favicon:
    return "/favicon.ico";
  case Path::Robots:
    return "/robots.txt";
  case Path::Other:
    return "/other";
  case Path::None:
    break;
  }
  return nullptr;
}

static const char *evidence_text(const Interaction &i) {
  switch (i.kind) {
  case Kind::Connect:
    return "connection_observed";
  case Kind::Access:
    return "canonical_access";
  case Kind::AuthAttempt:
    return "auth_attempt";
  case Kind::Input:
    return "input_observed";
  case Kind::Disconnect:
    if (i.close == Close::IdleTimeout)
      return "idle_timeout";
    return i.close == Close::Limit ? "capacity_limit" : "peer_closed";
  }
  return "connection_observed";
}

bool format_details(const Interaction &i, char *out, size_t capacity) {
  if (!out || !capacity)
    return false;
  *out = 0;
  // El contrato exige puertos 1..65535 y, para Telnet, puerto 23 sin ruta.
  if (!i.source_port || !i.destination_port)
    return false;
  if (i.protocol == Protocol::Telnet &&
      (i.destination_port != 23 || i.path != Path::None))
    return false;
  char address[16];
  snprintf(address, sizeof(address), "%u.%u.%u.%u", i.ipv4[0], i.ipv4[1],
           i.ipv4[2], i.ipv4[3]);
  const char *path = path_text(i.path);
  char path_json[24];
  if (path)
    snprintf(path_json, sizeof(path_json), "\"%s\"", path);
  else
    snprintf(path_json, sizeof(path_json), "null");
  int n = snprintf(out, capacity,
                   "{\"source_ipv4\":\"%s\",\"source_port\":%u,"
                   "\"destination_port\":%u,\"protocol\":\"%s\","
                   "\"interaction\":\"%s\",\"path\":%s,\"count\":1,"
                   "\"window_ms\":%u,\"suspicion\":\"informational\","
                   "\"evidence\":[\"%s\"]}",
                   address, unsigned(i.source_port),
                   unsigned(i.destination_port), protocol_text(i.protocol),
                   kind_text(i.kind), path_json, unsigned(IsolatedWindowMs),
                   evidence_text(i));
  if (n <= 0 || size_t(n) >= capacity) {
    *out = 0;
    return false;
  }
  return true;
}

bool publishable(const Interaction &i) {
  return i.kind == Kind::Access || i.kind == Kind::AuthAttempt ||
         i.kind == Kind::Input;
}

// Escritura volátil: el compilador no puede eliminar el borrado.
void wipe(void *data, size_t length) {
  volatile uint8_t *p = static_cast<volatile uint8_t *>(data);
  while (length--)
    *p++ = 0;
}

bool InteractionBus::push(const Interaction &item) {
  std::lock_guard<std::mutex> guard(lock_);
  if (closed_)
    return false;
  if (count_ >= BusCapacity) {
    ++dropped_;
    return false;
  }
  items_[(head_ + count_) % BusCapacity] = item;
  ++count_;
  return true;
}

bool InteractionBus::front(Interaction &out) const {
  std::lock_guard<std::mutex> guard(lock_);
  if (!count_)
    return false;
  out = items_[head_];
  return true;
}

void InteractionBus::pop() {
  std::lock_guard<std::mutex> guard(lock_);
  if (!count_)
    return;
  head_ = (head_ + 1) % BusCapacity;
  --count_;
}

void InteractionBus::open() {
  std::lock_guard<std::mutex> guard(lock_);
  head_ = count_ = 0;
  closed_ = false;
}

void InteractionBus::close() {
  std::lock_guard<std::mutex> guard(lock_);
  head_ = count_ = 0;
  closed_ = true;
}

size_t InteractionBus::size() const {
  std::lock_guard<std::mutex> guard(lock_);
  return count_;
}

uint32_t InteractionBus::dropped() const {
  std::lock_guard<std::mutex> guard(lock_);
  return dropped_;
}

} // namespace sentinel::cebo
