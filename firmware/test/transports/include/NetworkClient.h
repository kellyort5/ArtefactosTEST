#pragma once
#include "IPAddress.h"
#include <cstddef>
#include <cstdint>
class NetworkClient {
  bool live = false;
public:
  void setTimeout(unsigned seconds);
  int connect(IPAddress, uint16_t, int32_t timeout);
  int available();
  int read(uint8_t *, size_t);
  size_t write(const uint8_t *, size_t);
  bool connected() { return live; }
  void stop();
};
