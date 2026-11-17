// Existing regressions keep their independent, volatile fixtures. They link
// the real core helper; only the NVS SDK boundary is replaced here.
#include "connectivity/nvs.h"
esp_err_t nvs_open(const char *, int mode, nvs_handle_t *handle) {
  *handle = 1;
  return mode == NVS_READONLY ? ESP_ERR_NVS_NOT_FOUND : ESP_OK;
}
void nvs_close(nvs_handle_t) {}
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *, size_t *) {
  return ESP_ERR_NVS_NOT_FOUND;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *, size_t) {
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) { return ESP_OK; }
