#include "detectors.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
namespace sentinel::escudo {
static bool mac(const uint8_t *a, const uint8_t *b) { return !memcmp(a, b, 6); }
static bool unicast(const uint8_t *a) {
  static const uint8_t zero[6] = {};
  return !(a[0] & 1) && !mac(a, zero);
}
static bool ssid(const Observation &a, const Observation &b) {
  return a.ssid_present && b.ssid_present && a.ssid_length &&
         a.ssid_length == b.ssid_length &&
         !memcmp(a.ssid, b.ssid, a.ssid_length);
}
const char *detector_name(Detector d) {
  constexpr const char *names[] = {"deauth_burst", "suspicious_disconnect",
                                   "evil_twin", "karma_indication"};
  return names[size_t(d)];
}
void Detectors::Window::add(uint64_t now, uint64_t span) {
  while (size && now - times[head] > span) {
    head = (head + 1) % times.size();
    --size;
  }
  if (size == times.size()) {
    head = (head + 1) % times.size();
    --size;
  }
  times[(head + size++) % times.size()] = now;
}
void Detectors::clear_evidence() {
  deauth = {};
  disconnect = {};
  disassoc = {};
  for (auto &v : recent)
    v = {};
  for (auto &v : peers)
    v = {};
  for (auto &v : requests)
    v = {};
  for (auto &v : karma)
    v = {};
  generation = 0;
  last_time = 0;
}
void Detectors::reset() {
  clear_evidence();
  selected = false;
}
void Detectors::select(const Observation &baseline) {
  clear_evidence();
  target = baseline;
  selected = baseline.bssid_known && unicast(baseline.bssid);
}
bool Detectors::duplicate(const Observation &o) {
  Seen *slot = &recent[0];
  for (auto &v : recent) {
    if (o.retry && v.used && o.uptime_ms >= v.frame.uptime_ms &&
        o.uptime_ms - v.frame.uptime_ms <= 1000 && o.kind == v.frame.kind &&
        o.sequence == v.frame.sequence &&
        mac(o.transmitter, v.frame.transmitter) &&
        mac(o.bssid, v.frame.bssid) && mac(o.destination, v.frame.destination))
      return true;
    if (!v.used || (slot->used && v.frame.uptime_ms < slot->frame.uptime_ms))
      slot = &v;
  }
  *slot = {true, o};
  return false;
}
void Detectors::publish(Detection d) {
  Cooldown *slot = &cooldowns[0];
  for (auto &c : cooldowns) {
    if (c.used && c.detector == d.detector && mac(c.bssid, d.network.bssid)) {
      if (d.network.uptime_ms < c.time ||
          d.network.uptime_ms - c.time < CooldownMs)
        return;
      slot = &c;
      break;
    }
    if (!c.used || (slot->used && c.time < slot->time))
      slot = &c;
  }
  slot->used = true;
  slot->detector = d.detector;
  memcpy(slot->bssid, d.network.bssid, 6);
  slot->time = d.network.uptime_ms;
  if (sink)
    sink(sink_arg, d);
}
void Detectors::ingest(const Observation &o) {
  if (!selected || !o.generation || !o.channel || o.channel > 14 ||
      o.ssid_length > 32)
    return;
  if (generation && o.generation == generation && o.uptime_ms < last_time)
    return; // stale source timestamp cannot extend a window or underflow it
  if (o.generation != generation) {
    clear_evidence();
    generation = o.generation;
  }
  last_time = o.uptime_ms;
  if (duplicate(o))
    return;
  if (o.kind == FrameKind::Deauth || o.kind == FrameKind::Disassoc) {
    if (!o.bssid_known || !mac(o.bssid, target.bssid) ||
        (!mac(o.transmitter, target.bssid) &&
         !mac(o.destination, target.bssid)))
      return;
    // SSID/security come from the selected baseline, never inferred from
    // reason.
    auto network = target;
    network.uptime_ms = o.uptime_ms;
    network.channel = o.channel;
    network.rssi = o.rssi;
    if (o.kind == FrameKind::Deauth) {
      deauth.add(o.uptime_ms, 1000);
      if (deauth.size >= 10)
        publish({Detector::Deauth,
                 network,
                 {},
                 false,
                 DeauthRate | PassiveOnly,
                 unsigned(deauth.size),
                 1000});
    }
    disconnect.add(o.uptime_ms, 5000);
    // An empty insertion is avoided: prune Disassoc history independently.
    while (disassoc.size &&
           o.uptime_ms - disassoc.times[disassoc.head] > 5000) {
      disassoc.head = (disassoc.head + 1) % disassoc.times.size();
      --disassoc.size;
    }
    if (o.kind == FrameKind::Disassoc)
      disassoc.add(o.uptime_ms, 5000);
    if (disconnect.size >= 5)
      publish({Detector::Disconnect,
               network,
               {},
               false,
               uint16_t(DisconnectSequence | PassiveOnly |
                        (disassoc.size ? DisassocRate : DeauthRate)),
               unsigned(disconnect.size),
               5000});
  } else if (o.kind == FrameKind::Beacon ||
             o.kind == FrameKind::ProbeResponse) {
    evil_twin(o);
  }
  if (o.kind == FrameKind::ProbeRequest || o.kind == FrameKind::ProbeResponse)
    karma_probe(o);
}
static int auth_strength(const char *s) {
  if (!strcmp(s, "open"))
    return 0;
  if (!strcmp(s, "wep"))
    return 1;
  if (!strcmp(s, "wpa_personal") || !strcmp(s, "wpa_wpa2_personal"))
    return 2;
  if (!strcmp(s, "wpa2_personal") || !strcmp(s, "wpa2_wpa3_personal"))
    return 3;
  if (!strcmp(s, "wpa3_personal"))
    return 4;
  return -1; // enterprise and unknown are not ordered against personal AKMs
}
static int cipher_strength(const char *s) {
  if (!strcmp(s, "none"))
    return 0;
  if (!strcmp(s, "wep"))
    return 1;
  if (!strcmp(s, "tkip"))
    return 2;
  if (!strcmp(s, "ccmp") || !strcmp(s, "gcmp"))
    return 3;
  return -1; // mixed/unknown cannot substantiate a downgrade
}
static int pmf_strength(const char *s) {
  if (!strcmp(s, "disabled"))
    return 0;
  if (!strcmp(s, "capable"))
    return 1;
  if (!strcmp(s, "required"))
    return 2;
  return -1;
}
void Detectors::compare(const Observation &base, const Observation &peer) {
  if (!ssid(base, peer) || mac(base.bssid, peer.bssid) ||
      base.uptime_ms > peer.uptime_ms ||
      peer.uptime_ms - base.uptime_ms > PeerWindowMs)
    return;
  bool channel = base.channel != peer.channel;
  bool signal = std::abs(int(base.rssi) - int(peer.rssi)) >= 20;
  int a = auth_strength(base.auth), b = auth_strength(peer.auth);
  int x = cipher_strength(base.cipher), y = cipher_strength(peer.cipher);
  bool weaker = (a >= 0 && b >= 0 && b < a) || (x >= 0 && y >= 0 && y < x) ||
                (!strcmp(base.auth, "enterprise") &&
                 (!strcmp(peer.auth, "open") || !strcmp(peer.auth, "wep")));
  int p = pmf_strength(base.pmf), q = pmf_strength(peer.pmf);
  bool pmf = p >= 0 && q >= 0 && q < p;
  if (!weaker && !pmf && !(channel && signal))
    return;
  Detection d;
  d.detector = Detector::EvilTwin;
  d.network = peer;
  d.related = true;
  memcpy(d.related_bssid, base.bssid, 6);
  d.count = 2;
  d.window_ms = PeerWindowMs;
  d.evidence = SameSsid | DifferentBssid | PassiveOnly;
  if (channel)
    d.evidence |= ChannelChange;
  if (signal)
    d.evidence |= RssiChange;
  if (weaker)
    d.evidence |= SecurityDowngrade;
  if (pmf)
    d.evidence |= PmfChange;
  publish(d);
}
void Detectors::evil_twin(const Observation &o) {
  if (!o.bssid_known || !unicast(o.bssid) || !mac(o.transmitter, o.bssid) ||
      !o.ssid_present || !o.ssid_length)
    return;
  if (mac(o.bssid, target.bssid)) {
    // Selected SSID is the identity under observation. A hidden/different SSID
    // never silently changes it. Security is advertised, not authenticated.
    if (!ssid(o, target))
      return;
    target = o;
    for (auto &v : peers)
      if (v.used && o.uptime_ms >= v.frame.uptime_ms &&
          o.uptime_ms - v.frame.uptime_ms <= PeerWindowMs) {
        auto peer = v.frame;
        // The later baseline completes this comparison; retain candidate fields
        // and use the current observation time for event identity/cooldown.
        peer.uptime_ms = o.uptime_ms;
        compare(target, peer);
      }
    return;
  }
  if (!ssid(o, target))
    return;
  Peer *slot = &peers[0];
  for (auto &v : peers) {
    if (v.used && mac(v.frame.bssid, o.bssid)) {
      slot = &v;
      break;
    }
    if (!v.used || (slot->used && v.frame.uptime_ms < slot->frame.uptime_ms))
      slot = &v;
  }
  *slot = {true, o};
  compare(target, o);
}
void Detectors::karma_probe(const Observation &o) {
  if (!o.ssid_present || !o.ssid_length || !unicast(o.transmitter))
    return;
  if (o.kind == FrameKind::ProbeRequest) {
    Request *slot = &requests[0];
    for (auto &v : requests) {
      if (v.used && mac(v.frame.transmitter, o.transmitter) &&
          ssid(v.frame, o)) {
        slot = &v;
        break;
      }
      if (!v.used || (slot->used && v.frame.uptime_ms < slot->frame.uptime_ms))
        slot = &v;
    }
    *slot = {true, o};
    return;
  }
  if (!o.bssid_known || !mac(o.transmitter, o.bssid) || !unicast(o.destination))
    return;
  Request *match = nullptr;
  for (auto &v : requests)
    if (v.used && ssid(v.frame, o) && mac(v.frame.transmitter, o.destination) &&
        v.frame.channel == o.channel && o.uptime_ms >= v.frame.uptime_ms &&
        o.uptime_ms - v.frame.uptime_ms <= ProbeMatchMs &&
        (!v.frame.bssid_known || mac(v.frame.bssid, o.bssid))) {
      match = &v;
      break;
    }
  // Unmatched responses may simply mean missed requests/off-channel listening.
  // They are insufficient evidence; never manufacture a Karma alert from them.
  if (!match)
    return;
  match->used = false;
  KarmaPeer *slot = &karma[0];
  for (auto &v : karma) {
    if (v.used && mac(v.bssid, o.bssid)) {
      slot = &v;
      break;
    }
    if (!v.used || (slot->used && v.started < slot->started))
      slot = &v;
  }
  if (!slot->used || !mac(slot->bssid, o.bssid) ||
      o.uptime_ms - slot->started > KarmaWindowMs) {
    *slot = {};
    slot->used = true;
    memcpy(slot->bssid, o.bssid, 6);
    slot->started = o.uptime_ms;
  }
  slot->target_seen |= ssid(o, target) || mac(o.bssid, target.bssid);
  bool known = false;
  for (size_t i = 0; i < slot->names; ++i)
    known |= slot->lengths[i] == o.ssid_length &&
             !memcmp(slot->ssids[i], o.ssid, o.ssid_length);
  if (!known && slot->names < 3) {
    memcpy(slot->ssids[slot->names], o.ssid, o.ssid_length);
    slot->lengths[slot->names++] = o.ssid_length;
  }
  if (slot->names < 3 || !slot->target_seen)
    return;
  Detection d;
  d.detector = Detector::Karma;
  d.network = o;
  d.related = !mac(o.bssid, target.bssid);
  if (d.related)
    memcpy(d.related_bssid, target.bssid, 6);
  d.evidence = MultipleSsids | PassiveOnly;
  d.count = slot->names;
  d.window_ms = KarmaWindowMs;
  publish(d);
}
} // namespace sentinel::escudo
