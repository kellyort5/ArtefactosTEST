#include "include/escudo_target.h"
#include "nvs.h"
#include <cstring>

namespace sentinel {
namespace {
constexpr uint8_t Header[] = {'S', 'N', 'E', 'T', 1, 0, 0, 0};
constexpr const char *Namespace = "escudo_target", *Key = "selection";
// Byte fields only: bounded, no ABI padding, timestamps or raw SDK structs.
struct Record {
  uint8_t header[8] = {};
  char ssid[33] = {};
  uint8_t bssid[6] = {}, channel = 0;
  uint8_t auth = 0, pairwise = 0, group = 0;
};
static_assert(sizeof(Record) == 51);
bool valid(const ScanNetwork &n) {
  static const uint8_t zero[6] = {};
  // Hidden and non-UTF8 SSIDs are valid SDK observations. No embedded length
  // can authorize reading beyond the 32-byte SSID plus its terminator.
  return strnlen(n.ssid, sizeof(n.ssid)) <= 32 && !(n.bssid[0] & 1) &&
         memcmp(n.bssid, zero, sizeof(zero)) && n.channel >= 1 &&
         n.channel <= 14 && n.auth <= 16 && n.pairwise_cipher <= 12 &&
         n.group_cipher <= 12;
}
} // namespace

EscudoTargetLoad load_escudo_target(ScanNetwork &out) {
  nvs_handle_t handle;
  auto status = nvs_open(Namespace, NVS_READONLY, &handle);
  if (status == ESP_ERR_NVS_NOT_FOUND)
    return EscudoTargetLoad::Missing;
  if (status != ESP_OK)
    return EscudoTargetLoad::Error;
  size_t size = 0;
  status = nvs_get_blob(handle, Key, nullptr, &size);
  if (status == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return EscudoTargetLoad::Missing;
  }
  Record record;
  if (status == ESP_OK && size == sizeof(record)) {
    status = nvs_get_blob(handle, Key, &record, &size);
    if (status == ESP_OK && (size != sizeof(record) ||
                            memcmp(record.header, Header, sizeof(Header))))
      status = ESP_FAIL;
  } else if (status == ESP_OK)
    status = ESP_FAIL;
  nvs_close(handle);
  if (status != ESP_OK)
    return EscudoTargetLoad::Error;
  ScanNetwork target;
  memcpy(target.ssid, record.ssid, sizeof(target.ssid));
  memcpy(target.bssid, record.bssid, sizeof(target.bssid));
  target.channel = record.channel;
  target.auth = record.auth;
  target.pairwise_cipher = record.pairwise;
  target.group_cipher = record.group;
  if (!valid(target))
    return EscudoTargetLoad::Error;
  out = target;
  return EscudoTargetLoad::Loaded;
}

bool save_escudo_target(const ScanNetwork &target) {
  if (!valid(target))
    return false;
  Record record;
  memcpy(record.header, Header, sizeof(Header));
  memcpy(record.ssid, target.ssid, strnlen(target.ssid, 32));
  memcpy(record.bssid, target.bssid, sizeof(record.bssid));
  record.channel = target.channel;
  record.auth = target.auth;
  record.pairwise = target.pairwise_cipher;
  record.group = target.group_cipher;
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
