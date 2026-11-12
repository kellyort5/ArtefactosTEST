#pragma once
#include <cstdint>
namespace sentinel::escudo {
class ChannelScheduler {
  uint16_t mask = 0;
  uint8_t target = 0, current = 0, visited = 0;
  uint64_t deadline = 0;

public:
  bool start(uint8_t selected, uint16_t allowed, uint64_t now);
  uint8_t due(uint64_t now) const;
  void moved(uint8_t channel, uint64_t now);
  uint8_t channel() const { return current; }
  bool away() const { return current != target; }
};
} // namespace sentinel::escudo
