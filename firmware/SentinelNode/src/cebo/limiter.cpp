// Límites compartidos por HTTP y Telnet: configuración aceptada y admisión de
// conexiones. Sin dependencias del SDK.
#include "include/cebo.hpp"

namespace sentinel::cebo {
namespace {
unsigned index_of(Protocol p) { return p == Protocol::Telnet ? 1 : 0; }
// Tope propio de cada servicio, que ninguna configuración puede superar.
unsigned cap_of(Protocol p) {
  return p == Protocol::Telnet ? TelnetMaxSessions : HttpMaxSockets;
}
} // namespace

bool valid(const Config &c) {
  return c.max_connections >= 1 && c.max_connections <= MaxConnections &&
         c.idle_timeout_ms >= MinIdleTimeoutMs &&
         c.idle_timeout_ms <= MaxIdleTimeoutMs;
}

void Limiter::configure(const Config &c) {
  std::lock_guard<std::mutex> guard(lock_);
  config_ = c;
}

Config Limiter::config() const {
  std::lock_guard<std::mutex> guard(lock_);
  return config_;
}

uint32_t Limiter::idle_timeout_ms() const {
  std::lock_guard<std::mutex> guard(lock_);
  return config_.idle_timeout_ms;
}

bool Limiter::admit(Protocol p) {
  std::lock_guard<std::mutex> guard(lock_);
  const unsigned total = active_[0] + active_[1];
  if (total >= config_.max_connections || active_[index_of(p)] >= cap_of(p)) {
    if (rejected_ != UINT32_MAX)
      ++rejected_;
    return false;
  }
  ++active_[index_of(p)];
  return true;
}

void Limiter::release(Protocol p) {
  std::lock_guard<std::mutex> guard(lock_);
  if (active_[index_of(p)])
    --active_[index_of(p)];
}

void Limiter::reset(Protocol p) {
  std::lock_guard<std::mutex> guard(lock_);
  active_[index_of(p)] = 0;
}

void Limiter::clear() {
  std::lock_guard<std::mutex> guard(lock_);
  active_[0] = active_[1] = 0;
  rejected_ = 0;
}

unsigned Limiter::active() const {
  std::lock_guard<std::mutex> guard(lock_);
  return active_[0] + active_[1];
}

unsigned Limiter::active(Protocol p) const {
  std::lock_guard<std::mutex> guard(lock_);
  return active_[index_of(p)];
}

uint32_t Limiter::rejected() const {
  std::lock_guard<std::mutex> guard(lock_);
  return rejected_;
}

} // namespace sentinel::cebo
