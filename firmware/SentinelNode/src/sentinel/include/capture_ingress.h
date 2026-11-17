#pragma once
#include "../../sentinel_api/include/module_api.hpp"
#include <atomic>
#include <cstring>
namespace sentinel {
// Portable callback logic. No allocation, JSON, flash, waiting or frame
// parsing. The SDK adapter supplies a management frame WITHOUT its four-byte
// FCS.
class CaptureIngress {
  std::atomic<uint32_t> counter{0}, active{0};
  std::atomic<uint64_t> lost{0};

public:
  uint32_t open() {
    auto next = counter.fetch_add(1) + 1;
    if (!next)
      next = counter.fetch_add(1) + 1;
    active.store(next);
    return next;
  }
  void close() { active.store(0); }
  uint32_t generation() const { return active.load(); }
  bool current(uint32_t g) const { return g && g == active.load(); }
  void drop() { ++lost; }
  uint64_t dropped() const { return lost.load(); }
  bool copy(const uint8_t *bytes, size_t length, bool rx_ok, uint8_t channel,
            int8_t rssi, uint64_t now, RadioEvent &out) {
    auto g = generation();
    if (!g)
      return false;
    if (!rx_ok || !bytes || length < 24 || length > sizeof(out.bytes) ||
        channel < 1 || channel > 14) {
      drop();
      return false;
    }
    if (bytes[0] & 0x0f) {
      drop();
      return false;
    }
    unsigned subtype = bytes[0] >> 4;
    if (subtype != 4 && subtype != 5 && subtype != 8 && subtype != 10 &&
        subtype != 12)
      return false;
    out = {};
    out.kind = RadioEvent::Kind::Frame;
    out.generation = g;
    out.uptime_ms = now;
    out.channel = channel;
    out.rssi = rssi;
    out.length = out.original_length = length;
    memcpy(out.bytes, bytes, length);
    return true;
  }
};
} // namespace sentinel
