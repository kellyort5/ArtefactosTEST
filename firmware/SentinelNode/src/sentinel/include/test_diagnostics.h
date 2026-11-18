#pragma once
#include "../../../SentinelConfig.h"
#include <cstdint>
#if SENTINEL_SERIAL_TEST_DISPLAY
#include <atomic>
#endif
namespace sentinel {
#if SENTINEL_SERIAL_TEST_DISPLAY
// Volatile boot-local count, no frames, identifiers, events or persisted data.
inline std::atomic<uint32_t> detector_input_total{0};
inline void note_detector_input() { detector_input_total.fetch_add(1, std::memory_order_relaxed); }
inline uint32_t detector_inputs() { return detector_input_total.load(std::memory_order_relaxed); }
#else
inline void note_detector_input() {}
inline uint32_t detector_inputs() { return 0; }
#endif
}
