#include "include/ble_storage.h"
#include "host/ble_store.h"
#include "nvs.h"
#include <cstdlib>
#include <cstring>
namespace sentinel {
namespace {
struct Rule {
  const char *prefix;
  size_t size;
  unsigned capacity;
  bool security;
};
const Rule rules[] = {
    {"our_sec_", sizeof(ble_store_value_sec), CONFIG_BT_NIMBLE_MAX_BONDS, true},
    {"peer_sec_", sizeof(ble_store_value_sec), CONFIG_BT_NIMBLE_MAX_BONDS,
     true},
    {"cccd_sec_", sizeof(ble_store_value_cccd), CONFIG_BT_NIMBLE_MAX_CCCDS,
     false},
    {"csfc_sec_", sizeof(ble_store_value_csfc), MYNEWT_VAL(BLE_STORE_MAX_CSFCS),
     false},
    {"local_irk_", sizeof(ble_store_value_local_irk),
     CONFIG_BT_NIMBLE_MAX_BONDS, false},
    {"rpa_rec_", sizeof(ble_store_value_rpa_rec), CONFIG_BT_NIMBLE_MAX_BONDS,
     false},
};
const Rule *rule_for(const char *key, unsigned &index) {
  for (const auto &rule : rules) {
    size_t n = strlen(rule.prefix);
    if (strncmp(key, rule.prefix, n))
      continue;
    const char *digits = key + n;
    if (*digits < '1' || *digits > '9')
      return nullptr;
    char *end = nullptr;
    unsigned long value = strtoul(digits, &end, 10);
    if (*end || value > rule.capacity)
      return nullptr;
    index = value;
    return &rule;
  }
  return nullptr;
}
} // namespace
esp_err_t ble_storage_preflight() {
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open("nimble_bond", NVS_READONLY, &handle);
  if (err == ESP_ERR_NVS_NOT_FOUND)
    return ESP_OK;
  if (err != ESP_OK)
    return err;
  nvs_iterator_t it = nullptr;
  err = nvs_entry_find("nvs", "nimble_bond", NVS_TYPE_ANY, &it);
  while (err == ESP_OK) {
    nvs_entry_info_t info = {};
    err = nvs_entry_info(it, &info);
    if (err != ESP_OK)
      break;
    unsigned index = 0;
    const Rule *rule = rule_for(info.key, index);
    if (info.type != NVS_TYPE_BLOB || !rule) {
      err = ESP_ERR_NOT_SUPPORTED;
      break;
    }
    size_t size = 0;
    err = nvs_get_blob(handle, info.key, nullptr, &size);
    if (err != ESP_OK)
      break;
    if (size != rule->size) {
      err = ESP_ERR_INVALID_SIZE;
      break;
    }
    if (rule->security) {
      ble_store_value_sec value = {};
      err = nvs_get_blob(handle, info.key, &value, &size);
      bool secure = err == ESP_OK && value.authenticated && value.sc &&
                    value.key_size == 16;
      auto bytes = reinterpret_cast<volatile unsigned char *>(&value);
      for (size_t n = 0; n < sizeof(value); ++n)
        bytes[n] = 0;
      if (!secure) {
        err = ESP_ERR_NOT_SUPPORTED;
        break;
      }
    }
    err = nvs_entry_next(&it);
  }
  nvs_release_iterator(it);
  nvs_close(handle);
  return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
}
} // namespace sentinel
