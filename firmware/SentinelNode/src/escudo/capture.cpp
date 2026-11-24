#include "include/capture.hpp"
#include <algorithm>
#include <cstring>
#include <initializer_list>
namespace sentinel::escudo {
bool Capture::running() const { return !strcmp(state, "running"); }
void Capture::account(uint64_t now) {
  if (now >= last_account) {
    if (running())
      counters.coverage_ms += now - last_account;
    else if (!strcmp(state, "paused"))
      counters.paused_ms += now - last_account;
  }
  last_account = now;
  radio_drops = std::max(radio_drops, context->resources().capture_dropped());
}
CaptureStats Capture::stats(uint64_t now) const {
  auto out = counters;
  out.dropped_frames += radio_drops;
  if (now >= last_account) {
    if (running())
      out.coverage_ms += now - last_account;
    else if (!strcmp(state, "paused"))
      out.paused_ms += now - last_account;
  }
  return out;
}
cJSON *Capture::coverage(uint64_t now) const {
  auto s = stats(now);
  auto j = cJSON_CreateObject();
  if (running())
    cJSON_AddNumberToObject(j, "channel", scheduler.channel());
  else
    cJSON_AddNullToObject(j, "channel");
  cJSON_AddStringToObject(
      j, "pause_reason",
      running() ? (scheduler.away() ? "channel_hop" : "none") : pause_reason);
  cJSON_AddNumberToObject(j, "started_uptime_ms", segment_start);
  cJSON_AddNumberToObject(
      j, "duration_ms",
      ever_started && now >= segment_start ? now - segment_start : 0);
  cJSON_AddNumberToObject(j, "dropped_frames", s.dropped_frames);
  cJSON_AddNumberToObject(j, "dropped_events", s.dropped_events);
  return j;
}
void Capture::coverage_event(uint64_t now) {
  if (!ever_started)
    return;
  char details[512];
  auto j = coverage(now);
  bool ok = cJSON_PrintPreallocated(j, details, sizeof(details), false);
  cJSON_Delete(j);
  if (!ok || !context->emit_event("escudo.coverage", "info", details))
    ++counters.dropped_events;
  last_coverage = now;
}
void Capture::metrics(uint64_t now, bool force) {
  if (!ever_started || now <= last_metrics ||
      (!force && now - last_metrics < 1000))
    return;
  account(now);
  auto s = stats(now);
  auto p = cJSON_CreateObject();
  cJSON_AddNumberToObject(p, "beacons", s.beacons);
  cJSON_AddNumberToObject(p, "deauth", s.deauth);
  cJSON_AddNumberToObject(p, "disassoc", s.disassoc);
  auto alerts = cJSON_AddObjectToObject(p, "alerts");
  size_t family = 0;
  for (auto key : {"deauth_burst", "suspicious_disconnect", "evil_twin",
                   "karma_indication"})
    cJSON_AddNumberToObject(alerts, key, s.alerts[family++]);
  if (running())
    cJSON_AddNumberToObject(p, "channel", scheduler.channel());
  else
    cJSON_AddNullToObject(p, "channel");
  cJSON_AddNumberToObject(p, "coverage_ms", s.coverage_ms);
  cJSON_AddNumberToObject(p, "paused_ms", s.paused_ms);
  cJSON_AddNumberToObject(p, "dropped_frames", s.dropped_frames);
  char values[768];
  bool ok = cJSON_PrintPreallocated(p, values, sizeof(values), false);
  cJSON_Delete(p);
  if (ok)
    context->publish_metrics(std::min<uint64_t>(now - last_metrics, UINT32_MAX),
                             values);
  last_metrics = now;
}
void Capture::report_error() {
  if (!context->emit_event(
          "system.status", "warning",
          "{\"status_code\":\"resource_error\",\"resource\":\"capture\"}"))
    ++counters.dropped_events;
}
bool Capture::start(uint8_t target, uint64_t now) {
  if (running())
    return true;
  if (driver)
    return false;
  auto mask = context->resources().allowed_channels();
  if (!scheduler.start(target, mask, now) ||
      !context->resources().set_channel(target)) {
    fail(now);
    return false;
  }
  account(now);
  if (ever_started)
    coverage_event(now);
  if (!context->resources().capture(true)) {
    fail(now);
    return false;
  }
  driver = true;
  generation = context->resources().capture_generation();
  if (!generation) {
    stop(now);
    fail(now);
    return false;
  }
  state = "running";
  pause_reason = "none";
  ever_started = true;
  segment_start = last_account = now;
  last_metrics = now;
  coverage_event(now);
  return true;
}
bool Capture::stop(uint64_t now, const char *reason, bool pause) {
  if (!driver && !strcmp(state, "stopped") && !pause)
    return true;
  bool was_error = !strcmp(state, "error");
  account(now);
  if (!was_error) {
    if (ever_started)
      coverage_event(now);
    metrics(now, true);
  }
  // Invalidamos primero la generación: una trama pendiente del callback viejo
  // no debe llegar a detectores, incluso si el driver tarda en confirmar parada.
  generation = 0;
  if (driver && !context->resources().capture(false)) {
    state = "error";
    pause_reason = "resource_error";
    if (!was_error) {
      segment_start = now;
      report_error();
    }
    return false;
  }
  driver = false;
  state = pause ? "paused" : "stopped";
  pause_reason = reason;
  segment_start = last_account = now;
  coverage_event(now);
  return true;
}
void Capture::fail(uint64_t now) {
  account(now);
  generation = 0;
  if (driver && context->resources().capture(false))
    driver = false;
  state = "error";
  pause_reason = "resource_error";
  segment_start = last_account = now;
  coverage_event(now);
  report_error();
}
void Capture::tick(uint64_t now) {
  account(now);
  if (running())
    if (auto next = scheduler.due(now)) {
      metrics(now, true);
      if (!context->resources().set_channel(next)) {
        fail(now);
        return;
      }
      scheduler.moved(next, now);
      segment_start = now;
    }
  metrics(now);
  if (ever_started && now >= last_coverage && now - last_coverage >= 5000)
    coverage_event(now);
}
void Capture::ingest(const RadioEvent &event) {
  // El transporte no decide validez ni detección: aquí se filtran épocas y
  // tramas antes de entregar observaciones al módulo.
  if (!running() || !generation || event.generation != generation) {
    ++counters.dropped_frames;
    return;
  }
  Observation observation;
  auto parsed = parse_frame(event, observation);
  if (parsed == ParseResult::Malformed) {
    ++counters.dropped_frames;
    return;
  }
  if (parsed != ParseResult::Valid)
    return;
  switch (observation.kind) {
  case FrameKind::Beacon:
    ++counters.beacons;
    break;
  case FrameKind::Deauth:
    ++counters.deauth;
    break;
  case FrameKind::Disassoc:
    ++counters.disassoc;
    break;
  case FrameKind::ProbeRequest:
    ++counters.probe_requests;
    break;
  case FrameKind::ProbeResponse:
    ++counters.probe_responses;
    break;
  }
  if (observer)
    observer(observer_context, observation);
}
} // namespace sentinel::escudo
