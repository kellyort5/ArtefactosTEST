#include "include/frame_parser.hpp"
#include <cstring>
namespace sentinel::escudo {
static uint16_t le16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
static bool individual(const uint8_t *p) {
  uint8_t any = 0;
  for (unsigned i = 0; i < 6; ++i)
    any |= p[i];
  return any && !(p[0] & 1);
}
struct Security {
  bool psk = false, sae = false, enterprise = false, unknown = false;
  const char *cipher = nullptr;
  uint16_t caps = 0;
};
static const char *suite_cipher(const uint8_t *p, const uint8_t oui[3]) {
  if (memcmp(p, oui, 3))
    return "unknown";
  switch (p[3]) {
  case 1:
  case 5:
    return "wep";
  case 2:
    return "tkip";
  case 4:
  case 10:
    return "ccmp";
  case 8:
  case 9:
    return "gcmp";
  default:
    return "unknown";
  }
}
static const char *combine(const char *a, const char *b) {
  if (!a)
    return b;
  if (!strcmp(a, "unknown") || !strcmp(b, "unknown"))
    return "unknown";
  return strcmp(a, b) ? "mixed" : a;
}
static bool security(const uint8_t *p, size_t n, bool rsn, Security &out) {
  const uint8_t ieee[3] = {0, 0x0f, 0xac}, microsoft[3] = {0, 0x50, 0xf2};
  auto oui = rsn ? ieee : microsoft;
  if (n < 8 || le16(p) != 1)
    return false;
  p += 2;
  n -= 2;
  out.cipher = suite_cipher(p, oui);
  p += 4;
  n -= 4;
  unsigned count = le16(p);
  p += 2;
  n -= 2;
  if (!count || count > n / 4)
    return false;
  for (unsigned i = 0; i < count; ++i) {
    out.cipher = combine(out.cipher, suite_cipher(p, oui));
    p += 4;
    n -= 4;
  }
  if (n < 2)
    return false;
  count = le16(p);
  p += 2;
  n -= 2;
  if (!count || count > n / 4)
    return false;
  for (unsigned i = 0; i < count; ++i) {
    if (memcmp(p, oui, 3))
      out.unknown = true;
    else if (p[3] == 2 || (rsn && (p[3] == 4 || p[3] == 6)))
      out.psk = true;
    else if (rsn && (p[3] == 8 || p[3] == 9))
      out.sae = true;
    else if (p[3] == 1 || (rsn && (p[3] == 3 || p[3] == 5 || p[3] == 11 ||
                                   p[3] == 12 || p[3] == 13)))
      out.enterprise = true;
    else
      out.unknown = true;
    p += 4;
    n -= 4;
  }
  if (n) {
    if (n < 2)
      return false;
    out.caps = le16(p);
    p += 2;
    n -= 2;
  }
  if (rsn) {
    if ((out.caps & 0x40) && !(out.caps & 0x80))
      return false;
    if (n) {
      if (n < 2)
        return false;
      count = le16(p);
      p += 2;
      n -= 2;
      if (count > n / 16)
        return false;
      p += count * 16;
      n -= count * 16;
    }
    if (n == 4) {
      p += 4;
      n -= 4;
    } // optional group-management cipher; not a data cipher
  }
  return n == 0;
}
static bool ies(const uint8_t *p, size_t n, Observation &out) {
  Security rsn, wpa;
  bool ds = false, ht = false;
  while (n) {
    if (n < 2)
      return false;
    unsigned id = p[0], len = p[1];
    p += 2;
    n -= 2;
    if (len > n)
      return false;
    if (id == 0) {
      if (out.ssid_present || len > 32)
        return false;
      out.ssid_present = true;
      out.ssid_length = len;
      memcpy(out.ssid, p, len);
    } else if (id == 3) {
      if (ds || len != 1 || p[0] < 1 || p[0] > 14)
        return false;
      ds = true;
      out.advertised_channel = p[0];
    } else if (id == 61) {
      if (ht || len != 22 || p[0] < 1 || p[0] > 14)
        return false;
      ht = true;
      if (!ds)
        out.advertised_channel = p[0];
    } else if (id == 48) {
      if (out.rsn_present || !security(p, len, true, rsn))
        return false;
      out.rsn_present = true;
    } else if (id == 221) {
      if (len < 3)
        return false;
      const uint8_t prefix[4] = {0, 0x50, 0xf2, 1};
      if (len >= 4 && !memcmp(p, prefix, 4)) {
        if (out.wpa_present || !security(p + 4, len - 4, false, wpa))
          return false;
        out.wpa_present = true;
      }
    }
    p += len;
    n -= len;
  }
  if (!out.ssid_present)
    return false;
  if (out.kind == FrameKind::ProbeRequest)
    return true; // a client's IE is not AP security evidence
  if (out.rsn_present) {
    out.rsn_capabilities = rsn.caps;
    out.pmf = rsn.caps & 0x40   ? "required"
              : rsn.caps & 0x80 ? "capable"
                                : "disabled";
    out.cipher = combine(rsn.cipher, out.wpa_present ? wpa.cipher : rsn.cipher);
    if (rsn.unknown || (out.wpa_present && wpa.unknown) ||
        ((rsn.enterprise || (out.wpa_present && wpa.enterprise)) &&
         (rsn.psk || rsn.sae || (out.wpa_present && wpa.psk))))
      out.auth = "unknown";
    else if (rsn.enterprise)
      out.auth = "enterprise";
    else if (rsn.psk && rsn.sae)
      out.auth = out.wpa_present ? "unknown" : "wpa2_wpa3_personal";
    else if (rsn.sae)
      out.auth = out.wpa_present ? "unknown" : "wpa3_personal";
    else if (rsn.psk)
      out.auth = out.wpa_present ? (wpa.psk ? "wpa_wpa2_personal" : "unknown")
                                 : "wpa2_personal";
  } else if (out.wpa_present) {
    out.pmf = "disabled";
    out.cipher = wpa.cipher;
    out.auth = wpa.unknown                  ? "unknown"
               : wpa.enterprise && !wpa.psk ? "enterprise"
               : wpa.psk && !wpa.enterprise ? "wpa_personal"
                                            : "unknown";
  } else if (!out.privacy) {
    out.auth = "open";
    out.cipher = "none";
    out.pmf = "disabled";
  }
  return true;
}
ParseResult parse_frame(const RadioEvent &e, Observation &out) {
  out = {};
  if (e.kind != RadioEvent::Kind::Frame)
    return ParseResult::Ignored;
  if (e.length != e.original_length || e.length < 24 ||
      e.length > sizeof(e.bytes) || e.channel < 1 || e.channel > 14)
    return ParseResult::Malformed;
  auto p = e.bytes;
  uint16_t fc = le16(p), sequence = le16(p + 22);
  if (fc & 0x000f || fc & 0x0300 || fc & 0x0400 || sequence & 0x000f)
    return ParseResult::Malformed;
  unsigned subtype = (fc >> 4) & 15;
  switch (subtype) {
  case 8:
    out.kind = FrameKind::Beacon;
    break;
  case 12:
    out.kind = FrameKind::Deauth;
    break;
  case 10:
    out.kind = FrameKind::Disassoc;
    break;
  case 4:
    out.kind = FrameKind::ProbeRequest;
    break;
  case 5:
    out.kind = FrameKind::ProbeResponse;
    break;
  default:
    return ParseResult::Ignored;
  }
  size_t header = 24 + (fc & 0x8000 ? 4 : 0);
  if (e.length < header)
    return ParseResult::Malformed;
  memcpy(out.destination, p + 4, 6);
  memcpy(out.transmitter, p + 10, 6);
  memcpy(out.bssid, p + 16, 6);
  out.bssid_known = individual(out.bssid);
  if (!individual(out.transmitter) ||
      (out.kind != FrameKind::ProbeRequest && !out.bssid_known))
    return ParseResult::Malformed;
  out.generation = e.generation;
  out.uptime_ms = e.uptime_ms;
  out.channel = e.channel;
  out.rssi = e.rssi > 0 ? 0 : e.rssi < -127 ? -127 : e.rssi;
  out.sequence = sequence >> 4;
  out.retry = fc & 0x0800;
  out.protected_frame = fc & 0x4000;
  p += header;
  size_t n = e.length - header;
  if (out.kind == FrameKind::Deauth || out.kind == FrameKind::Disassoc) {
    if (n < 2)
      return ParseResult::Malformed;
    if (!out.protected_frame) {
      out.reason = le16(p);
      out.reason_known = true;
    }
    return ParseResult::Valid;
  }
  if (out.protected_frame)
    return ParseResult::Malformed; // no decryption or invented IE fields
  if (out.kind != FrameKind::ProbeRequest) {
    if (n < 12)
      return ParseResult::Malformed;
    out.privacy = le16(p + 10) & 0x10;
    p += 12;
    n -= 12;
  }
  return ies(p, n, out) ? ParseResult::Valid : ParseResult::Malformed;
}
} // namespace sentinel::escudo
