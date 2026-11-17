#pragma once
#include "../../sentinel_api/include/module_api.hpp"
namespace sentinel::escudo {
enum class FrameKind { Beacon, Deauth, Disassoc, ProbeRequest, ProbeResponse };
struct Observation {
  FrameKind kind = FrameKind::Beacon;
  uint32_t generation = 0;
  uint64_t uptime_ms = 0;
  uint8_t destination[6] = {}, transmitter[6] = {}, bssid[6] = {};
  uint8_t channel = 0, advertised_channel = 0;
  int8_t rssi = 0;
  uint16_t sequence = 0, reason = 0, rsn_capabilities = 0;
  bool retry = false, protected_frame = false, reason_known = false,
       bssid_known = false;
  bool ssid_present = false, privacy = false, rsn_present = false,
       wpa_present = false;
  uint8_t ssid[32] = {}, ssid_length = 0;
  const char *auth = "unknown", *cipher = "unknown", *pmf = "unknown";
};
enum class ParseResult { Valid, Ignored, Malformed };
// Complete copied management frame, already stripped of FCS by the manager.
// Never retains pointers to a RadioEvent or unbounded information elements.
ParseResult parse_frame(const RadioEvent &, Observation &);
} // namespace sentinel::escudo
