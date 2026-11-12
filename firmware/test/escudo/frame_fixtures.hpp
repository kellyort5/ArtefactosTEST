#pragma once
#include "module_api.hpp"
#include <cassert>
#include <cstring>
#include <vector>
namespace fixtures {
using Bytes = std::vector<uint8_t>;
inline Bytes rsn(uint8_t akm = 2, uint16_t caps = 0x80) {
  return {1,
          0,
          0,
          0x0f,
          0xac,
          4,
          1,
          0,
          0,
          0x0f,
          0xac,
          4,
          1,
          0,
          0,
          0x0f,
          0xac,
          akm,
          uint8_t(caps),
          uint8_t(caps >> 8)};
}
inline void ie(Bytes &b, uint8_t id, const Bytes &value) {
  assert(value.size() <= 255);
  b.push_back(id);
  b.push_back(value.size());
  b.insert(b.end(), value.begin(), value.end());
}
inline Bytes management(uint8_t subtype, bool privacy = false,
                        unsigned id = 1) {
  Bytes b(24, 0);
  b[0] = subtype << 4;
  memset(b.data() + 4, 0xff, 6);
  b[10] = b[16] = 2;
  b[15] = b[21] = id;
  b[22] = 0x30;
  if (subtype == 8 || subtype == 5) {
    b.resize(36, 0);
    b[32] = 100;
    b[34] = privacy ? 0x10 : 0;
  }
  return b;
}
inline Bytes beacon(bool secure = false, unsigned id = 1, uint8_t channel = 6) {
  auto b = management(8, secure, id);
  ie(b, 0, {'L', 'A', 'B'});
  ie(b, 3, {channel});
  if (secure)
    ie(b, 48, rsn());
  return b;
}
inline Bytes deauth(bool protected_frame = false, uint8_t subtype = 12) {
  auto b = management(subtype);
  b.push_back(7);
  b.push_back(0);
  if (protected_frame) {
    b[1] |= 0x40;
    b.resize(b.size() + 16, 0x55);
  }
  return b;
}
inline Bytes probe(bool response = false) {
  auto b = management(response ? 5 : 4, false, response ? 1 : 2);
  if (!response)
    memset(b.data() + 16, 0xff, 6);
  ie(b, 0, response ? Bytes{'L', 'A', 'B'} : Bytes{});
  return b;
}
inline sentinel::RadioEvent frame(const Bytes &b, uint32_t generation = 1,
                                  uint8_t channel = 6, uint64_t now = 1000) {
  assert(b.size() <= 512);
  sentinel::RadioEvent e = {};
  e.kind = sentinel::RadioEvent::Kind::Frame;
  e.generation = generation;
  e.channel = channel;
  e.rssi = -50;
  e.uptime_ms = now;
  e.length = e.original_length = b.size();
  memcpy(e.bytes, b.data(), b.size());
  return e;
}
} // namespace fixtures
