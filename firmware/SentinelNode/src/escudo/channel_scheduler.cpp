#include "include/channel_scheduler.hpp"
namespace sentinel::escudo {
bool ChannelScheduler::start(uint8_t selected, uint16_t allowed, uint64_t now) {
  if (selected < 1 || selected > 14 || !(allowed & (1U << (selected - 1))))
    return false;
  mask = allowed & 0x3fff;
  target = current = visited = selected;
  deadline = now + 800;
  return true;
}
uint8_t ChannelScheduler::due(uint64_t now) const {
  if (now < deadline)
    return 0;
  if (current != target)
    return target;
  for (unsigned step = 1; step <= 14; ++step) {
    uint8_t next = (visited - 1 + step) % 14 + 1;
    if (next != target && (mask & (1U << (next - 1))))
      return next;
  }
  return 0;
}
void ChannelScheduler::moved(uint8_t next, uint64_t now) {
  current = next;
  if (next != target)
    visited = next;
  // Charge actual elapsed time; never fabricate catch-up slots after a delay.
  deadline = now + (next == target ? 800 : 200);
}
} // namespace sentinel::escudo
