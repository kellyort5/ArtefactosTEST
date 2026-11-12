#include "frame_fixtures.hpp"
#include "test_support.hpp"
#include <array>
using namespace fixtures;
static void ingress_tests() {
  CaptureIngress ingress;
  RadioEvent output;
  auto b = beacon();
  assert(!ingress.copy(b.data(), b.size(), true, 6, -50, 10, output));
  auto first = ingress.open();
  assert(first && ingress.copy(b.data(), b.size(), true, 6, -50, 10, output));
  assert(output.generation == first && output.length == b.size());
  assert(!ingress.copy(b.data(), b.size(), false, 6, -50, 10, output));
  assert(!ingress.copy(b.data(), 23, true, 6, -50, 10, output));
  assert(!ingress.copy(b.data(), 513, true, 6, -50, 10,
                       output)); // rejects before any source read
  assert(!ingress.copy(b.data(), b.size(), true, 0, -50, 10, output));
  auto drops = ingress.dropped();
  assert(drops == 4);
  ingress.close();
  assert(!ingress.current(first));
  auto second = ingress.open();
  assert(second != first && !ingress.current(first));
  assert(ingress.copy(b.data(), b.size(), true, 6, -50, 10, output) &&
         output.generation == second);
  auto other = management(0);
  assert(!ingress.copy(other.data(), other.size(), true, 6, -50, 10, output) &&
         ingress.dropped() == drops);
  std::deque<RadioEvent> queue;
  for (unsigned i = 0; i < RadioQueueSize + 1; ++i) {
    assert(ingress.copy(b.data(), b.size(), true, 6, -50, 10, output));
    if (queue.size() == RadioQueueSize)
      ingress.drop();
    else
      queue.push_back(output);
  }
  assert(queue.size() == RadioQueueSize && ingress.dropped() == drops + 1);
}
static void parser_tests() {
  Observation o;
  auto check = [&](const Bytes &b) { return parse_frame(frame(b), o); };
  assert(check(beacon()) == ParseResult::Valid && o.kind == FrameKind::Beacon &&
         o.ssid_length == 3 && o.bssid_known);
  assert(!strcmp(o.auth, "open") && !strcmp(o.cipher, "none") &&
         o.advertised_channel == 6);
  assert(check(beacon(true)) == ParseResult::Valid &&
         !strcmp(o.auth, "wpa2_personal") && !strcmp(o.pmf, "capable"));
  auto b = management(8, true);
  ie(b, 0, {});
  ie(b, 48, rsn(8, 0xc0));
  assert(check(b) == ParseResult::Valid && o.ssid_present && !o.ssid_length &&
         !strcmp(o.auth, "wpa3_personal") && !strcmp(o.pmf, "required"));
  auto mixed = rsn();
  mixed[12] = 2;
  mixed.insert(mixed.begin() + 18, {0, 0x0f, 0xac, 8});
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, mixed);
  assert(check(b) == ParseResult::Valid &&
         !strcmp(o.auth, "wpa2_wpa3_personal"));
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, rsn(1));
  assert(check(b) == ParseResult::Valid && !strcmp(o.auth, "enterprise"));
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, rsn(18));
  assert(check(b) == ParseResult::Valid && !strcmp(o.auth, "unknown"));
  auto wpa = rsn(2, 0);
  for (size_t i : {size_t(2), size_t(8), size_t(14)}) {
    wpa[i] = 0;
    wpa[i + 1] = 0x50;
    wpa[i + 2] = 0xf2;
  }
  Bytes vendor = {0, 0x50, 0xf2, 1};
  vendor.insert(vendor.end(), wpa.begin(), wpa.end());
  b = management(8, true);
  ie(b, 0, {'W'});
  ie(b, 221, vendor);
  assert(check(b) == ParseResult::Valid && !strcmp(o.auth, "wpa_personal"));
  ie(b, 48, rsn());
  assert(check(b) == ParseResult::Valid &&
         !strcmp(o.auth, "wpa_wpa2_personal"));
  assert(check(deauth()) == ParseResult::Valid && o.kind == FrameKind::Deauth &&
         o.reason_known && o.reason == 7);
  assert(check(deauth(false, 10)) == ParseResult::Valid &&
         o.kind == FrameKind::Disassoc);
  assert(check(deauth(true)) == ParseResult::Valid && o.protected_frame &&
         !o.reason_known);
  assert(check(probe()) == ParseResult::Valid &&
         o.kind == FrameKind::ProbeRequest && !o.bssid_known &&
         o.ssid_present && !o.ssid_length);
  assert(check(probe(true)) == ParseResult::Valid &&
         o.kind == FrameKind::ProbeResponse && o.bssid_known);
  b = beacon();
  b[1] |= 8;
  assert(check(b) == ParseResult::Valid && o.retry && o.sequence == 3);
  auto event = frame(b);
  event.original_length++;
  assert(parse_frame(event, o) == ParseResult::Malformed);
  event = frame(b);
  event.length = event.original_length = 513;
  assert(parse_frame(event, o) == ParseResult::Malformed);
  b = beacon();
  b[1] |= 4;
  assert(check(b) == ParseResult::Malformed);
  b = beacon();
  b[22] |= 1;
  assert(check(b) == ParseResult::Malformed);
  b = beacon();
  b[1] |= 0x80;
  b.insert(b.begin() + 24, 4, 0);
  assert(check(b) == ParseResult::Valid);
  b = beacon();
  ie(b, 0, {'D'});
  assert(check(b) == ParseResult::Malformed);
  b = beacon();
  b.push_back(48);
  b.push_back(20);
  b.push_back(1);
  assert(check(b) == ParseResult::Malformed);
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, rsn(2, 0x40));
  assert(check(b) == ParseResult::Malformed);
  auto bad = rsn();
  bad[6] = bad[7] = 0xff;
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, bad);
  assert(check(b) == ParseResult::Malformed);
  auto full = rsn();
  full.insert(full.end(), {1, 0});
  full.resize(full.size() + 15, 0);
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, full);
  assert(check(b) == ParseResult::Malformed);
  full = rsn();
  full.insert(full.end(), {0, 0, 0, 0x0f, 0xac, 6});
  b = management(8, true);
  ie(b, 0, {'X'});
  ie(b, 48, full);
  assert(check(b) == ParseResult::Valid);
  for (size_t length = 0; length < 20; ++length) {
    b = management(8, true);
    ie(b, 0, {'X'});
    b.push_back(48);
    b.push_back(20);
    auto body = rsn();
    b.insert(b.end(), body.begin(), body.begin() + length);
    assert(check(b) == ParseResult::Malformed);
  }
}
static void scheduler_tests() {
  ChannelScheduler s;
  auto mask = uint16_t((1 << 0) | (1 << 5) | (1 << 10));
  assert(s.start(6, mask, 0));
  assert(!s.due(799));
  assert(s.due(800) == 11);
  s.moved(11, 800);
  assert(!s.due(999) && s.due(1000) == 6);
  s.moved(6, 1000);
  assert(s.due(1800) == 1);
  s.moved(1, 1800);
  assert(s.due(2000) == 6);
  assert(!s.start(14, mask, 0) && !s.start(0, mask, 0) && !s.start(6, 0, 0));
  assert(s.start(6, 1 << 5, 0) && !s.due(10000));
}
static void coverage_tests() {
  Harness h;
  h.radio.channels = (1 << 0) | (1 << 5) | (1 << 10);
  Capture capture;
  capture.init(h.context);
  assert(capture.start(6, h.context.now));
  auto gen = capture.epoch();
  capture.ingest(frame(beacon(), gen));
  capture.ingest(frame(deauth(), gen));
  capture.ingest(frame(deauth(false, 10), gen));
  capture.ingest(frame(probe(), gen));
  capture.ingest(frame(probe(true), gen));
  auto counts = capture.stats(h.context.now);
  assert(counts.beacons == 1 && counts.deauth == 1 && counts.disassoc == 1 &&
         counts.probe_requests == 1 && counts.probe_responses == 1);
  h.context.now = 1800;
  capture.tick(h.context.now);
  assert(h.radio.channel == 11 &&
         capture.stats(h.context.now).coverage_ms == 800);
  h.context.now = 2000;
  assert(capture.stop(h.context.now, "channel_hop", true));
  h.context.now = 2500;
  assert(capture.stats(h.context.now).paused_ms == 500);
  assert(capture.start(6, h.context.now) && capture.epoch() != gen);
  capture.ingest(frame(beacon(), gen));
  auto bad = frame(beacon(), capture.epoch());
  bad.original_length++;
  capture.ingest(bad);
  h.radio.ingress.drop();
  h.context.now = 2600;
  capture.tick(h.context.now);
  auto totals = capture.stats(h.context.now);
  assert(totals.coverage_ms == 1100 && totals.paused_ms == 500 &&
         totals.dropped_frames == 3);
  h.context.sink_ready = false;
  h.context.now = 2700;
  assert(capture.stop(h.context.now));
  assert(capture.stats(h.context.now).dropped_events > 0);
  h.context.sink_ready = true;
  h.context.now = 8000;
  capture.tick(h.context.now);
  assert(capture.stats(h.context.now).coverage_ms == 1200);
  h.radio.channels = 0;
  assert(!capture.start(6, h.context.now) && !h.radio.physical_capture);
}
static const char *target_a =
    "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:01\",\"channel\":6}";
static const char *target_b =
    "{\"ssid\":\"LAB\",\"bssid\":\"02:00:00:00:00:02\",\"channel\":11}";
static void integration_tests() {
  Harness h;
  std::vector<Observation> observed;
  h.module.observe(
      [](void *arg, const Observation &o) {
        auto v = static_cast<std::vector<Observation> *>(arg);
        assert(v->size() < 32);
        v->push_back(o);
      },
      &observed);
  assert(!h.command("escudo.scan").error);
  h.radio.records = {network(1), network(2, "LAB", 11)};
  h.radio.done();
  h.tick();
  assert(!h.command("escudo.select_target", target_a).error);
  h.tick();
  assert(!h.command("escudo.capture_start").error);
  auto old = h.radio.capture_generation();
  assert(old && h.radio.physical_capture);
  h.radio.feed(frame(beacon(true), old, 6, h.context.now));
  h.radio.feed(frame(deauth(), old, 6, h.context.now));
  h.tick();
  assert(observed.size() == 2 && observed[0].kind == FrameKind::Beacon);
  auto copied = frame(beacon(), old, 6, h.context.now);
  assert(!h.command("escudo.capture_stop").error && !h.radio.physical_capture);
  assert(!h.command("escudo.capture_start").error &&
         h.radio.capture_generation() != old);
  h.radio.events.push_back(copied);
  h.tick();
  assert(observed.size() == 2);
  old = h.radio.capture_generation();
  assert(!h.command("escudo.select_target", target_b).error);
  h.radio.events.push_back(frame(beacon(), old, 6, h.context.now));
  h.radio.feed(frame(beacon(false, 2, 11), 1, 11, h.context.now));
  h.tick();
  assert(observed.size() == 3 && observed.back().bssid[5] == 2 &&
         observed.back().channel == 11);
  assert(h.radio.channel == 11);
  h.context.now += 800;
  h.tick();
  assert(h.radio.channel == 1);
  h.radio.feed(frame(beacon(false, 3, 1), 1, 1, h.context.now));
  h.tick();
  assert(observed.back().channel == 1);
  h.context.now += 200;
  h.tick();
  assert(h.radio.channel == 11);
  h.state();
  auto before_pause = h.radio.capture_generation();
  assert(!h.command("escudo.scan").error && !h.radio.physical_capture);
  ModuleSnapshot snapshot;
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "\"capture_status\":\"paused\""));
  h.radio.events.push_back(frame(beacon(), before_pause, 11, h.context.now));
  h.radio.done();
  h.tick();
  assert(h.radio.physical_capture &&
         h.radio.capture_generation() != before_pause);
  assert(!h.command("escudo.scan").error);
  assert(!h.command("escudo.capture_stop").error);
  h.radio.done();
  h.tick();
  assert(!h.radio.physical_capture);
  assert(!h.command("escudo.capture_start").error);
  h.radio.capture_stop_ok = false;
  assert(!strcmp(h.command("escudo.capture_stop").error, "resource_error"));
  auto n = observed.size();
  h.radio.events.push_back(frame(beacon(), old, 6, h.context.now));
  h.tick();
  assert(observed.size() == n);
  h.radio.capture_stop_ok = true;
  assert(!h.command("escudo.capture_stop").error);
  assert(!h.command("escudo.capture_start").error);
  h.radio.channel_ok = false;
  h.context.now += 800;
  h.tick();
  h.module.snapshot(snapshot);
  assert(strstr(snapshot.state, "\"capture_status\":\"error\""));
  h.radio.channel_ok = true;
  assert(!h.command("escudo.capture_stop").error);
  assert(!h.command("escudo.capture_start").error);
  h.radio.capture_stop_ok = false;
  assert(!h.supervisor.transition(Mode::Idle, h.context.now) &&
         h.supervisor.transitioning());
  h.context.now += 5000;
  h.tick();
  assert(!strcmp(h.runtime.error, "timeout") &&
         h.runtime.profile == Mode::Escudo);
  h.radio.capture_stop_ok = true;
  assert(!h.supervisor.transition(Mode::Idle, h.context.now));
  assert(!h.radio.physical_capture);
  h.state();
}
int main(int argc, char **argv) {
  emit_wire = argc == 2 && !strcmp(argv[1], "wire");
  if (!emit_wire) {
    ingress_tests();
    parser_tests();
    scheduler_tests();
    coverage_tests();
  }
  integration_tests();
  if (!emit_wire)
    std::cout << "Capture callback epochs/bounds, management/IE/RSN parser, "
                 "scheduler, BLE commands, metrics/coverage, rescan and "
                 "failed-stop isolation: PASS (host)\n";
}
