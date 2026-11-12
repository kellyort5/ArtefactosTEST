#pragma once
#include "frame_parser.hpp"
#include <array>
namespace sentinel::escudo {
enum class Detector : size_t { Deauth, Disconnect, EvilTwin, Karma };
enum Evidence : uint16_t {
  DeauthRate = 1,
  DisassocRate = 2,
  DisconnectSequence = 4,
  SameSsid = 8,
  DifferentBssid = 16,
  ChannelChange = 32,
  RssiChange = 64,
  SecurityDowngrade = 128,
  PmfChange = 256,
  MultipleSsids = 512,
  PassiveOnly = 1024
};
struct Detection {
  Detector detector = Detector::Deauth;
  Observation network;
  uint8_t related_bssid[6] = {};
  bool related = false;
  uint16_t evidence = PassiveOnly;
  unsigned count = 0, window_ms = 0;
};
using DetectionSink = void (*)(void *, const Detection &);
// Pure bounded state machine. Called only by the module/supervisor, never the
// Wi-Fi callback. No packet transmission, dynamic allocation or raw retention.
class Detectors {
  struct Window {
    std::array<uint64_t, 64> times{};
    size_t head = 0, size = 0;
    void add(uint64_t now, uint64_t span);
  } deauth, disconnect, disassoc;
  struct Cooldown {
    bool used = false;
    Detector detector = Detector::Deauth;
    uint8_t bssid[6] = {};
    uint64_t time = 0;
  } cooldowns[16] = {};
  struct Seen {
    bool used = false;
    Observation frame;
  } recent[32] = {};
  struct Peer {
    bool used = false;
    Observation frame;
  } peers[16] = {};
  struct Request {
    bool used = false;
    Observation frame;
  } requests[16] = {};
  struct KarmaPeer {
    bool used = false, target_seen = false;
    uint8_t bssid[6] = {};
    uint64_t started = 0;
    uint8_t ssids[3][32] = {}, lengths[3] = {};
    size_t names = 0;
  } karma[8] = {};
  Observation target;
  bool selected = false;
  uint32_t generation = 0;
  uint64_t last_time = 0;
  DetectionSink sink = nullptr;
  void *sink_arg = nullptr;
  void clear_evidence();
  bool duplicate(const Observation &);
  void publish(Detection);
  void compare(const Observation &, const Observation &);
  void evil_twin(const Observation &);
  void karma_probe(const Observation &);

public:
  static constexpr uint64_t CooldownMs = 30000, PeerWindowMs = 30000,
                            KarmaWindowMs = 10000, ProbeMatchMs = 2000;
  void observe(DetectionSink f, void *arg) {
    sink = f;
    sink_arg = arg;
  }
  void select(const Observation &baseline);
  void reset(); // clear evidence/selection, preserve per-boot cooldowns
  void ingest(const Observation &);
};
const char *detector_name(Detector);
} // namespace sentinel::escudo
