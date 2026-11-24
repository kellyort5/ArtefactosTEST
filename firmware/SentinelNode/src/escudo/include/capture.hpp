#pragma once
#include "channel_scheduler.hpp"
#include "frame_parser.hpp"
namespace sentinel::escudo {
struct CaptureStats {
  uint64_t alerts[4] = {};
  uint64_t beacons = 0, deauth = 0, disassoc = 0, probe_requests = 0,
           probe_responses = 0;
  uint64_t coverage_ms = 0, paused_ms = 0, dropped_frames = 0,
           dropped_events = 0;
};
using ObservationSink = void (*)(void *, const Observation &);
class Capture {
  ModuleContext *context = nullptr;
  ChannelScheduler scheduler;
  CaptureStats counters;
  uint64_t radio_drops = 0;
  const char *state = "stopped", *pause_reason = "stopped";
  bool driver = false, ever_started = false;
  uint32_t generation = 0;
  uint64_t last_account = 0, segment_start = 0, last_metrics = 0,
           last_coverage = 0;
  ObservationSink observer = nullptr;
  void *observer_context = nullptr;
  void account(uint64_t);
  void report_error();
  void coverage_event(uint64_t);
  void metrics(uint64_t, bool force = false);

public:
  void init(ModuleContext &c) { context = &c; }
  void observe(ObservationSink sink, void *arg) {
    observer = sink;
    observer_context = arg;
  }
  bool start(uint8_t target, uint64_t now);
  bool stop(uint64_t now, const char *reason = "stopped", bool pause = false);
  void fail(uint64_t now);
  void tick(uint64_t now);
  void ingest(const RadioEvent &);
  void detection(size_t family, bool delivered) {
    if (family < 4)
      ++counters.alerts[family];
    if (!delivered)
      ++counters.dropped_events;
  }
  bool running() const;
  bool needs_release() const { return driver; }
  const char *status() const { return state; }
  uint32_t epoch() const { return generation; }
  CaptureStats stats(uint64_t now) const;
  cJSON *coverage(uint64_t now) const;
};
} // namespace sentinel::escudo
