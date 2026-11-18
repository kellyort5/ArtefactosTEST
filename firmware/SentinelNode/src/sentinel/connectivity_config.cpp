#include "include/connectivity_config.h"
#include "nvs.h"
#include <cstring>

namespace sentinel {
namespace {
// Fixed size/version: reject incompatible installs, never migrate or format NVS.
constexpr unsigned char Header[8] = {'S', 'N', 'C', 'F', 2, 0, 0, 0};
struct Record {
  unsigned char header[sizeof(Header)] = {};
  ConnectivityConfig config;
};
static_assert(sizeof(Record) == sizeof(Header) + sizeof(ConnectivityConfig));
constexpr const char *Namespace = "connectivity", *Key = "config";

// Count Unicode scalar values, as the runtime schemas do, but also reject broken
// UTF-8 from storage. Bound every read before calling any C string parser.
template <size_t N>
bool text(const char (&s)[N], size_t min, size_t max, bool bytes = false) {
  size_t length = strnlen(s, N), count = 0;
  if (length == N)
    return false;
  for (size_t i = 0; i < length; ++i) {
    uint32_t cp = static_cast<unsigned char>(s[i]), lowest = 0;
    unsigned tail = 0;
    if (cp >= 0xc2 && cp <= 0xdf) {
      tail = 1;
      cp &= 31;
      lowest = 0x80;
    } else if (cp >= 0xe0 && cp <= 0xef) {
      tail = 2;
      cp &= 15;
      lowest = 0x800;
    } else if (cp >= 0xf0 && cp <= 0xf4) {
      tail = 3;
      cp &= 7;
      lowest = 0x10000;
    } else if (cp >= 0x80)
      return false;
    if (i + tail >= length)
      return false;
    while (tail--) {
      auto next = static_cast<unsigned char>(s[++i]);
      if ((next & 0xc0) != 0x80)
        return false;
      cp = (cp << 6) | (next & 63);
    }
    if (cp < lowest || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
      return false;
    ++count;
  }
  size_t size = bytes ? length : count;
  return size >= min && size <= max;
}
template <size_t N>
bool copy(char (&out)[N], const cJSON *args, const char *key) {
  const char *s = str(args, key);
  if (!s || strnlen(s, N) == N)
    return false;
  memcpy(out, s, strlen(s) + 1);
  return true;
}
} // namespace

bool ConnectivityConfig::valid() const {
  return text(ssid, 1, 32, true) && text(wifi_password, 0, 63, true) &&
         text(uri, 1, 256, true) && mqtt_uri_valid(uri) &&
         text(username, 1, 64) && text(password, 1, 128);
}
bool ConnectivityConfig::from_args(const cJSON *args) {
  return fields(args, "ssid,wifi_password,mqtt_uri,mqtt_username,mqtt_password") &&
         copy(ssid, args, "ssid") && copy(wifi_password, args, "wifi_password") &&
         copy(uri, args, "mqtt_uri") && copy(username, args, "mqtt_username") &&
         copy(password, args, "mqtt_password") &&
         valid();
}
ConnectivityLoad load_connectivity(ConnectivityConfig &out) {
  nvs_handle_t handle;
  auto status = nvs_open(Namespace, NVS_READONLY, &handle);
  if (status == ESP_ERR_NVS_NOT_FOUND)
    return ConnectivityLoad::Missing;
  if (status != ESP_OK)
    return ConnectivityLoad::Error;
  size_t size = 0;
  status = nvs_get_blob(handle, Key, nullptr, &size);
  if (status == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return ConnectivityLoad::Missing;
  }
  Record record;
  if (status == ESP_OK && size == sizeof(record)) {
    status = nvs_get_blob(handle, Key, &record, &size);
    if (status == ESP_OK && (size != sizeof(record) ||
        memcmp(record.header, Header, sizeof(Header)) || !record.config.valid()))
      status = ESP_FAIL;
  } else if (status == ESP_OK)
    status = ESP_FAIL;
  nvs_close(handle);
  if (status != ESP_OK)
    return ConnectivityLoad::Error;
  out = record.config;
  return ConnectivityLoad::Loaded;
}
bool save_connectivity(const ConnectivityConfig &config) {
  if (!config.valid())
    return false;
  Record record;
  memcpy(record.header, Header, sizeof(Header));
  record.config = config;
  nvs_handle_t handle;
  if (nvs_open(Namespace, NVS_READWRITE, &handle) != ESP_OK)
    return false;
  auto status = nvs_set_blob(handle, Key, &record, sizeof(record));
  if (status == ESP_OK)
    status = nvs_commit(handle);
  nvs_close(handle);
  return status == ESP_OK;
}
} // namespace sentinel
