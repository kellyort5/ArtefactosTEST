#include "detectors.hpp"
#include "frame_fixtures.hpp"
#include "test_support.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace sentinel::escudo;
using namespace fixtures;
static Observation parsed(const Bytes &b, uint64_t now = 1000,
                          uint8_t channel = 6) {
  Observation o;
  assert(parse_frame(frame(b, 1, channel, now), o) == ParseResult::Valid);
  return o;
}
struct Rig {
  Detectors detectors;
  std::vector<Detection> hits;
  Rig() {
    detectors.observe(
        [](void *p, const Detection &d) {
          static_cast<Rig *>(p)->hits.push_back(d);
        },
        this);
    detectors.select(parsed(beacon(true)));
  }
  size_t count(Detector d) const {
    size_t n = 0;
    for (auto &v : hits)
      if (v.detector == d)
        ++n;
    return n;
  }
  void feed(const Bytes &b, uint64_t t) { detectors.ingest(parsed(b, t)); }
};
static void disconnect_tests() {
  Rig r;
  for (unsigned i = 0; i < 9; ++i)
    r.feed(deauth(), 1000 + i * 10);
  assert(r.count(Detector::Deauth) == 0 && r.count(Detector::Disconnect) == 1);
  r.feed(deauth(), 1090);
  assert(r.count(Detector::Deauth) == 1 && r.hits.back().count == 10);
  r.feed(deauth(), 1100);
  assert(r.count(Detector::Deauth) == 1);
  // Boundaries/expiry use the capture clock; no replay or sleeping.
  for (unsigned i = 0; i < 10; ++i)
    r.feed(deauth(), 31090 + i);
  assert(r.count(Detector::Deauth) == 2);
  Rig slow;
  for (unsigned i = 0; i < 12; ++i)
    slow.feed(deauth(), 1000 + i * 1100);
  assert(slow.count(Detector::Deauth) == 0);
  Rig mixed;
  for (unsigned i = 0; i < 4; ++i)
    mixed.feed(deauth(false, 10), 1000 + i * 100);
  assert(mixed.hits.empty());
  mixed.feed(deauth(), 1500);
  assert(mixed.count(Detector::Disconnect) == 1 &&
         (mixed.hits[0].evidence & DisassocRate));
  Rig expired;
  for (unsigned i = 0; i < 4; ++i)
    expired.feed(deauth(false, 10), 1000 + i);
  expired.feed(deauth(false, 10), 6004);
  assert(expired.hits.empty());
  Rig retries;
  retries.feed(deauth(), 1000);
  auto retry = parsed(deauth(), 1001);
  retry.retry = true;
  for (unsigned i = 0; i < 20; ++i)
    retries.detectors.ingest(retry);
  assert(retries.hits.empty());
  Rig unrelated;
  auto other = parsed(deauth());
  other.bssid[5] = 2;
  for (unsigned i = 0; i < 20; ++i)
    unrelated.detectors.ingest(other);
  assert(unrelated.hits.empty());
  Rig epoch;
  for (unsigned i = 0; i < 4; ++i)
    epoch.feed(deauth(), 1000 + i);
  auto next = parsed(deauth(), 1010);
  next.generation = 2;
  epoch.detectors.ingest(next);
  assert(epoch.hits.empty());
  next.uptime_ms = 900;
  epoch.detectors.ingest(next);
  assert(epoch.hits.empty());
}
static void lifecycle_tests() {
  Rig r;
  for (unsigned i = 0; i < 10; ++i)
    r.feed(deauth(), 1000 + i);
  assert(r.count(Detector::Deauth) == 1);
  r.detectors.reset(); // mode exit/entry must not bypass cooldown
  r.detectors.select(parsed(beacon(true), 2000));
  for (unsigned i = 0; i < 10; ++i)
    r.feed(deauth(), 2000 + i);
  assert(r.count(Detector::Deauth) == 1);
  // A different monitored BSSID has an independent key.
  r.detectors.select(parsed(beacon(true, 2), 2100));
  auto o = parsed(deauth(), 2100);
  o.bssid[5] = o.transmitter[5] = 2;
  for (unsigned i = 0; i < 10; ++i) {
    o.uptime_ms = 2100 + i;
    r.detectors.ingest(o);
  }
  assert(r.count(Detector::Deauth) == 2);
  Rig boundary;
  for (unsigned i = 0; i < 9; ++i)
    boundary.feed(deauth(), 1000);
  boundary.feed(deauth(), 2000);
  assert(boundary.count(Detector::Deauth) == 1);
  Rig outside;
  for (unsigned i = 0; i < 9; ++i)
    outside.feed(deauth(), 1000);
  outside.feed(deauth(), 2001);
  assert(outside.count(Detector::Deauth) == 0);
  Rig full;
  for (unsigned i = 0; i < 80; ++i)
    full.feed(deauth(), 1000 + i);
  for (auto &hit : full.hits)
    assert(hit.count <= 64);
}
static void evil_tests() {
  Rig r;
  r.feed(beacon(true, 2), 1100);
  assert(r.hits.empty()); // same SSID benign
  r.detectors.ingest(parsed(beacon(true, 2, 11), 1200, 11));
  assert(r.hits.empty()); // channel alone
  auto peer = parsed(beacon(true, 2), 1300);
  peer.rssi = -20;
  r.detectors.ingest(peer);
  assert(r.hits.empty()); // RSSI alone
  peer.channel = 11;
  peer.uptime_ms = 1400;
  r.detectors.ingest(peer);
  assert(r.count(Detector::EvilTwin) == 1 &&
         (r.hits.back().evidence & ChannelChange) &&
         (r.hits.back().evidence & RssiChange));
  r.feed(beacon(false, 3), 1500);
  assert(r.count(Detector::EvilTwin) == 2 &&
         (r.hits.back().evidence & SecurityDowngrade));
  r.feed(beacon(false, 3), 1600);
  assert(r.count(Detector::EvilTwin) == 2);
  Rig pmf;
  auto base = parsed(beacon(true));
  base.pmf = "required";
  pmf.detectors.select(base);
  pmf.feed(beacon(true, 2), 1100);
  assert(pmf.count(Detector::EvilTwin) == 1 &&
         (pmf.hits[0].evidence & PmfChange));
  Rig unknown;
  auto u = parsed(beacon(true, 2), 1100);
  u.auth = u.cipher = u.pmf = "unknown";
  unknown.detectors.ingest(u);
  assert(unknown.hits.empty());
  Rig expired;
  expired.feed(beacon(false, 2), 31001);
  assert(expired.hits.empty()); // stale baseline is not evidence
  // Candidate arriving before a fresh target Beacon is considered on baseline
  // refresh.
  expired.feed(beacon(true), 31002);
  assert(expired.count(Detector::EvilTwin) == 1);
  Rig different;
  auto p = parsed(beacon(false, 2), 1100);
  p.ssid[0] = 'X';
  different.detectors.ingest(p);
  assert(different.hits.empty());
}
static Bytes named_probe(const char *name, bool response, unsigned ap = 4,
                         unsigned client = 9) {
  auto b = management(response ? 5 : 4, false, response ? ap : client);
  if (response) {
    memset(b.data() + 4, 0, 6);
    b[4] = 2;
    b[9] = client;
  } else
    memset(b.data() + 16, 0xff, 6);
  Bytes n(name, name + strlen(name));
  ie(b, 0, n);
  return b;
}
static void pair(Rig &r, const char *name, uint64_t t, unsigned ap = 4,
                 unsigned client = 9) {
  r.feed(named_probe(name, false, ap, client), t);
  r.feed(named_probe(name, true, ap, client), t + 1);
}
static void karma_tests() {
  Rig r;
  pair(r, "LAB", 1000);
  pair(r, "OTHER", 1100);
  assert(r.count(Detector::Karma) == 0);
  pair(r, "THIRD", 1200);
  assert(r.count(Detector::Karma) == 1);
  assert(r.hits.back().count == 3 && (r.hits.back().evidence & MultipleSsids));
  pair(r, "FOURTH", 1300);
  assert(r.count(Detector::Karma) == 1);
  Rig benign;
  for (unsigned i = 0; i < 5; ++i)
    pair(benign, "LAB", 1000 + i * 100);
  assert(benign.count(Detector::Karma) == 0);
  Rig no_request;
  for (auto name : {"LAB", "OTHER", "THIRD"})
    no_request.feed(named_probe(name, true), 1000);
  assert(no_request.count(Detector::Karma) == 0);
  Rig delayed;
  for (unsigned i = 0; i < 3; ++i) {
    const char *name = i == 0 ? "LAB" : i == 1 ? "OTHER" : "THIRD";
    delayed.feed(named_probe(name, false), 1000 + i * 3000);
    delayed.feed(named_probe(name, true), 3001 + i * 3000);
  }
  assert(delayed.count(Detector::Karma) == 0);
  Rig wrong_client;
  for (auto name : {"LAB", "OTHER", "THIRD"}) {
    wrong_client.feed(named_probe(name, false), 1000);
    wrong_client.feed(named_probe(name, true, 4, 10), 1001);
  }
  assert(wrong_client.count(Detector::Karma) == 0);
  Rig old;
  pair(old, "LAB", 1000);
  pair(old, "OTHER", 1100);
  pair(old, "THIRD", 11002);
  assert(old.count(Detector::Karma) == 0);
  Rig unrelated;
  pair(unrelated, "A", 1000);
  pair(unrelated, "B", 1100);
  pair(unrelated, "C", 1200);
  assert(unrelated.count(Detector::Karma) == 0);
  Rig offchannel;
  for (auto name : {"LAB", "OTHER", "THIRD"}) {
    offchannel.feed(named_probe(name, false), 1000);
    offchannel.detectors.ingest(parsed(named_probe(name, true), 1001, 11));
  }
  assert(offchannel.count(Detector::Karma) == 0);
}
static size_t events_of(const Harness &h, const char *detector) {
  size_t n = 0;
  for (auto &wire : h.context.events) {
    auto j = cJSON_Parse(wire.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    auto details = cJSON_GetObjectItemCaseSensitive(p, "details");
    auto value = cJSON_GetObjectItemCaseSensitive(details, "detector");
    if (cJSON_IsString(value) && !strcmp(value->valuestring, detector)) {
      ++n;
      assert(!strcmp(text(p, "origin"), "simulated"));
      assert(!strcmp(text(j, "time_quality"), "unknown"));
      assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(j, "timestamp")));
    }
    cJSON_Delete(j);
  }
  return n;
}
static void integration_tests() {
  Harness h;
  assert(!h.command("escudo.scan").error);
  h.radio.records = {network(1)};
  h.radio.done();
  h.tick();
  assert(
      !h.command(
            "escudo.select_target",
            "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}")
           .error);
  h.tick();
  assert(!h.command("escudo.capture_start").error);
  auto feed = [&](Bytes b) {
    h.context.now += 10;
    h.radio.feed(frame(b, 1, 6, h.context.now));
    h.tick();
  };
  feed(beacon(true));
  feed(beacon(true, 2));
  assert(events_of(h, "evil_twin") == 0);
  for (unsigned i = 0; i < 9; ++i)
    feed(deauth());
  assert(events_of(h, "deauth_burst") == 0);
  feed(deauth());
  assert(events_of(h, "deauth_burst") == 1);
  assert(events_of(h, "suspicious_disconnect") == 1);
  feed(beacon(false, 2));
  assert(events_of(h, "evil_twin") == 1);
  for (auto name : {"LAB", "OTHER", "THIRD"}) {
    feed(named_probe(name, false));
    feed(named_probe(name, true));
  }
  assert(events_of(h, "karma_indication") == 1);
  auto before = events_of(h, "evil_twin");
  feed(beacon(false, 2));
  assert(events_of(h, "evil_twin") == before);
  // A denied/full shared output is counted without blocking or a private queue.
  h.context.sink_ready = false;
  feed(beacon(false, 5));
  h.context.sink_ready = true;
  h.context.now += 1000;
  h.tick();
  h.state();
  assert(!h.context.metrics.empty());
  auto metric = cJSON_Parse(h.context.metrics.back().c_str());
  auto values = cJSON_GetObjectItemCaseSensitive(
      cJSON_GetObjectItemCaseSensitive(metric, "payload"), "values");
  auto alerts = cJSON_GetObjectItemCaseSensitive(values, "alerts");
  for (auto name : {"deauth_burst", "suspicious_disconnect", "evil_twin",
                    "karma_indication"})
    assert(cJSON_GetObjectItemCaseSensitive(alerts, name)->valuedouble >= 1);
  cJSON_Delete(metric);
  bool dropped = false;
  for (auto &wire : h.context.events) {
    auto j = cJSON_Parse(wire.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    if (!strcmp(text(p, "name"), "escudo.coverage")) {
      auto details = cJSON_GetObjectItemCaseSensitive(p, "details");
      dropped |= cJSON_GetObjectItemCaseSensitive(details, "dropped_events")
                     ->valuedouble > 0;
    }
    cJSON_Delete(j);
  }
  // Coverage reports losses periodically or at stop; checked below.
  ModuleSnapshot state;
  h.module.snapshot(state);
  assert(state.capabilities.detectors);
  // New generation clears partial windows, but cumulative counters persist.
  assert(!h.command("escudo.capture_stop").error);
  auto stale = frame(deauth(), h.radio.capture_generation(), 6, h.context.now);
  assert(!h.command("escudo.capture_start").error);
  auto detections = h.context.events.size();
  h.radio.events.push_back(stale);
  h.tick();
  assert(h.context.events.size() == detections);
  h.context.now += 100; // a new interval must elapse before the closing metric
  h.tick();
  assert(!h.command("escudo.capture_stop").error);
  h.state();
  for (auto &wire : h.context.events) {
    auto j = cJSON_Parse(wire.c_str());
    auto p = cJSON_GetObjectItemCaseSensitive(j, "payload");
    if (!strcmp(text(p, "name"), "escudo.coverage")) {
      auto details = cJSON_GetObjectItemCaseSensitive(p, "details");
      dropped |= cJSON_GetObjectItemCaseSensitive(details, "dropped_events")
                     ->valuedouble > 0;
    }
    cJSON_Delete(j);
  }
  assert(dropped);
  assert(h.context.metrics.size() >= 2);
  auto last = cJSON_Parse(h.context.metrics.back().c_str());
  auto p = cJSON_GetObjectItemCaseSensitive(last, "payload");
  auto v = cJSON_GetObjectItemCaseSensitive(p, "values");
  auto a = cJSON_GetObjectItemCaseSensitive(v, "alerts");
  assert(cJSON_GetObjectItemCaseSensitive(a, "deauth_burst")->valuedouble == 1);
  cJSON_Delete(last);
}
int main(int argc, char **argv) {
  emit_wire = argc == 2 && !strcmp(argv[1], "wire");
  if (emit_wire) {
    integration_tests();
    return 0;
  }
  disconnect_tests();
  lifecycle_tests();
  evil_tests();
  karma_tests();
  integration_tests();
  std::cout
      << "All four detectors: thresholds, expiry, cooldown, retries/epochs, "
         "benign duplicates, security/channel/RSSI, passive Karma correlation "
         "and inconclusive cases: PASS (host)\n";
}
