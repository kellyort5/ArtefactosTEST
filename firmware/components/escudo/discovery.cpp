#include "escudo.hpp"
#include <cstdio>
#include <cstring>
#ifdef ESP_PLATFORM
#include "esp_wifi_types.h"
// The public ScanNetwork fields carry pinned ESP-IDF enum values. Fail the
// build on an SDK mapping change, rather than silently labelling another
// auth/cipher.
static_assert(WIFI_AUTH_OPEN == 0 && WIFI_AUTH_WEP == 1 &&
              WIFI_AUTH_WPA_PSK == 2 && WIFI_AUTH_WPA2_PSK == 3 &&
              WIFI_AUTH_WPA_WPA2_PSK == 4 && WIFI_AUTH_ENTERPRISE == 5 &&
              WIFI_AUTH_WPA3_PSK == 6 && WIFI_AUTH_WPA2_WPA3_PSK == 7);
static_assert(WIFI_AUTH_WPA3_ENT_192 == 10 && WIFI_AUTH_WPA3_EXT_PSK == 11 &&
              WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE == 12 &&
              WIFI_AUTH_WPA3_ENTERPRISE == 14 &&
              WIFI_AUTH_WPA2_WPA3_ENTERPRISE == 15 &&
              WIFI_AUTH_WPA_ENTERPRISE == 16);
static_assert(WIFI_CIPHER_TYPE_NONE == 0 && WIFI_CIPHER_TYPE_WEP40 == 1 &&
              WIFI_CIPHER_TYPE_WEP104 == 2 && WIFI_CIPHER_TYPE_TKIP == 3 &&
              WIFI_CIPHER_TYPE_CCMP == 4 && WIFI_CIPHER_TYPE_TKIP_CCMP == 5 &&
              WIFI_CIPHER_TYPE_GCMP == 8 && WIFI_CIPHER_TYPE_GCMP256 == 9);
#endif
namespace sentinel::escudo {
static const char *string(const cJSON *j, const char *key) {
  auto v = cJSON_GetObjectItemCaseSensitive(j, key);
  return cJSON_IsString(v) ? v->valuestring : nullptr;
}
const char *auth_label(unsigned auth) {
  switch (auth) {
  case 0:
    return "open";
  case 1:
    return "wep";
  case 2:
    return "wpa_personal";
  case 3:
    return "wpa2_personal";
  case 4:
    return "wpa_wpa2_personal";
  case 5:
  case 10:
  case 14:
  case 15:
  case 16:
    return "enterprise";
  case 6:
  case 11:
  case 12:
    return "wpa3_personal";
  case 7:
    return "wpa2_wpa3_personal";
  default:
    return "unknown";
  }
}
static const char *cipher(unsigned c) {
  switch (c) {
  case 0:
    return "none";
  case 1:
  case 2:
    return "wep";
  case 3:
    return "tkip";
  case 4:
    return "ccmp";
  case 5:
    return "mixed";
  case 8:
  case 9:
    return "gcmp";
  default:
    return "unknown";
  }
}
const char *cipher_label(unsigned a, unsigned b) {
  auto x = cipher(a), y = cipher(b);
  if (!strcmp(x, "unknown") || !strcmp(y, "unknown"))
    return "unknown";
  return strcmp(x, y) ? "mixed" : x;
}
static bool utf8(const uint8_t *s, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    unsigned c = s[i];
    if (c < 0x20 || c == 0x7f)
      return false;
    if (c < 0x80)
      continue;
    unsigned remaining = 0;
    uint32_t cp = 0, min = 0;
    if (c >= 0xc2 && c <= 0xdf) {
      remaining = 1;
      cp = c & 31;
      min = 0x80;
    } else if (c >= 0xe0 && c <= 0xef) {
      remaining = 2;
      cp = c & 15;
      min = 0x800;
    } else if (c >= 0xf0 && c <= 0xf4) {
      remaining = 3;
      cp = c & 7;
      min = 0x10000;
    } else
      return false;
    if (i + remaining >= n)
      return false;
    while (remaining--) {
      c = s[++i];
      if ((c & 0xc0) != 0x80)
        return false;
      cp = (cp << 6) | (c & 63);
    }
    if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
      return false;
  }
  return true;
}
static cJSON *ssid_json(const ScanNetwork &n) {
  size_t bytes = strnlen(n.ssid, 32);
  if (utf8(reinterpret_cast<const uint8_t *>(n.ssid), bytes))
    return cJSON_CreateString(n.ssid);
  char hex[65] = {};
  for (size_t i = 0; i < bytes; ++i)
    snprintf(hex + 2 * i, 3, "%02x", uint8_t(n.ssid[i]));
  auto result = cJSON_CreateObject();
  cJSON_AddStringToObject(result, "encoding", "hex");
  cJSON_AddStringToObject(result, "value", hex);
  return result;
}
static bool ssid_matches(const cJSON *j, const ScanNetwork &n) {
  size_t bytes = strnlen(n.ssid, 32);
  if (cJSON_IsString(j))
    return strlen(j->valuestring) == bytes &&
           !memcmp(j->valuestring, n.ssid, bytes);
  auto encoding = string(j, "encoding"), value = string(j, "value");
  if (!encoding || strcmp(encoding, "hex") || !value ||
      strlen(value) != 2 * bytes)
    return false;
  char hex[65] = {};
  for (size_t i = 0; i < bytes; ++i)
    snprintf(hex + 2 * i, 3, "%02x", uint8_t(n.ssid[i]));
  return !strcmp(value, hex);
}
static void bssid(const ScanNetwork &n, char out[18]) {
  snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", n.bssid[0], n.bssid[1],
           n.bssid[2], n.bssid[3], n.bssid[4], n.bssid[5]);
}
void Catalog::reset(const char *id) {
  used = overflow = issued = 0;
  snprintf(scan, sizeof(scan), "%s", id ? id : "");
}
void Catalog::add(const ScanNetwork &raw, uint64_t seen) {
  if (!raw.channel || raw.channel > 14)
    return;
  size_t at = 0;
  for (; at < used; ++at)
    if (!memcmp(entries[at].network.bssid, raw.bssid, 6))
      break;
  if (at == CatalogCapacity) {
    ++overflow;
    return;
  }
  if (at == used)
    ++used;
  entries[at] = {raw, seen};
  auto &n = entries[at].network;
  n.ssid[32] = 0;
  if (n.rssi > 0)
    n.rssi = 0;
  if (n.rssi < -127)
    n.rssi = -127;
}
cJSON *Catalog::target_json(const Entry &e) {
  auto j = cJSON_CreateObject();
  char mac[18];
  bssid(e.network, mac);
  cJSON_AddItemToObject(j, "ssid", ssid_json(e.network));
  cJSON_AddStringToObject(j, "bssid", mac);
  cJSON_AddNumberToObject(j, "channel", e.network.channel);
  return j;
}
cJSON *Catalog::network_json(const Entry &e) {
  auto j = target_json(e);
  auto &n = e.network;
  cJSON_AddNumberToObject(j, "rssi", n.rssi);
  cJSON_AddStringToObject(j, "auth", auth_label(n.auth));
  cJSON_AddStringToObject(j, "cipher",
                          cipher_label(n.pairwise_cipher, n.group_cipher));
  cJSON_AddStringToObject(j, "pmf", "unknown");
  cJSON_AddNumberToObject(j, "seen_uptime_ms", e.seen);
  return j;
}
const Entry *Catalog::find(const cJSON *target) const {
  auto mac = string(target, "bssid");
  auto channel = cJSON_GetObjectItemCaseSensitive(target, "channel");
  if (!mac || !cJSON_IsNumber(channel))
    return nullptr;
  for (size_t i = 0; i < used; ++i) {
    char value[18];
    bssid(entries[i].network, value);
    if (!strcmp(mac, value) &&
        channel->valuedouble == entries[i].network.channel &&
        ssid_matches(cJSON_GetObjectItemCaseSensitive(target, "ssid"),
                     entries[i].network))
      return &entries[i];
  }
  return nullptr;
}
const char *Catalog::page(const cJSON *after, size_t limit, cJSON *&result) {
  result = nullptr;
  if (!*scan)
    return "invalid_state";
  if (!limit || limit > 4 || !after)
    return "invalid_args";
  size_t offset = 0;
  if (!cJSON_IsNull(after)) {
    if (!cJSON_IsString(after))
      return "invalid_args";
    auto s = after->valuestring;
    size_t prefix = strlen(scan);
    if (strncmp(s, scan, prefix) || s[prefix] != ':')
      return "revision_conflict";
    s += prefix + 1;
    if (!*s || *s == '0')
      return "invalid_args";
    for (; *s; ++s) {
      if (*s < '0' || *s > '9')
        return "invalid_args";
      offset = offset * 10 + *s - '0';
      if (offset > CatalogCapacity)
        return "invalid_args";
    }
    if (offset >= used || !(issued & (1ULL << offset)))
      return "invalid_args";
  }
  // A conservative payload budget leaves room for the largest v2 envelope,
  // ACK correlation IDs and result kind. Reduce entries, never their fields.
  for (size_t take = limit; take; --take) {
    result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "scan_id", scan);
    auto networks = cJSON_AddArrayToObject(result, "networks");
    size_t end = offset + take;
    if (end > used)
      end = used;
    for (size_t i = offset; i < end; ++i)
      cJSON_AddItemToArray(networks, network_json(entries[i]));
    if (end < used) {
      char next[80];
      snprintf(next, sizeof(next), "%s:%u", scan, unsigned(end));
      cJSON_AddStringToObject(result, "next", next);
    } else
      cJSON_AddNullToObject(result, "next");
    cJSON_AddBoolToObject(result, "more", end < used);
    char bounded[1281];
    if (cJSON_PrintPreallocated(result, bounded, sizeof(bounded), false)) {
      if (end < used)
        issued |= 1ULL << end;
      return nullptr;
    }
    cJSON_Delete(result);
    result = nullptr;
  }
  return "capacity_exceeded";
}
} // namespace sentinel::escudo
