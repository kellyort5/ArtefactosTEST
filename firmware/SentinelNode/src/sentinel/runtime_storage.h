#pragma once
#include "include/core.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <cstdio>

namespace sentinel {
// Adaptador privado del journal: solo el supervisor escribe en event_log.
// La identidad y los vínculos BLE pertenecen a la NVS principal; un fallo aquí
// se propaga al journal, sin borrar ninguna partición para intentar
// recuperarla.
class NvsStore : public Store {
  nvs_handle_t handle = 0;
  esp_err_t last = ESP_OK;

public:
  bool ready = false;
  bool init() {
    last = nvs_flash_init_partition("event_log");
    if (last != ESP_OK)
      return false;
    last = nvs_open_from_partition("event_log", "ring", NVS_READWRITE, &handle);
    ready = last == ESP_OK;
    return ready;
  }
  bool read(size_t slot, char *out, size_t *n) override {
    if (!ready) {
      last = ESP_FAIL;
      return false;
    }
    char key[16];
    snprintf(key, sizeof(key), "slot%02u", static_cast<unsigned>(slot));
    last = nvs_get_blob(handle, key, out, n);
    return last == ESP_OK;
  }
  bool missing() const override { return last == ESP_ERR_NVS_NOT_FOUND; }
  bool write(size_t slot, const char *data, size_t n) override {
    if (!ready)
      return false;
    char key[16];
    snprintf(key, sizeof(key), "slot%02u", static_cast<unsigned>(slot));
    last = nvs_set_blob(handle, key, data, n);
    if (last == ESP_OK)
      last = nvs_commit(handle);
    return last == ESP_OK;
  }
};
} // namespace sentinel
