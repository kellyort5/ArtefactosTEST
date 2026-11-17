#include "include/arduino_environment.h"
#include "nvs_flash.h"
// Arduino's standard startup retries/formats these two errors before setup().
// The menu's linker wrapper prevents that destructive branch while preserving
// the exact first result for identity initialization in the existing runtime.
static esp_err_t first_status = ESP_ERR_INVALID_STATE;
static bool observed = false;
extern "C" esp_err_t __real_nvs_flash_init(void);
extern "C" esp_err_t __wrap_nvs_flash_init(void) {
  if (!observed) {
    first_status = __real_nvs_flash_init();
    observed = true;
  }
  if (first_status == ESP_ERR_NVS_NO_FREE_PAGES ||
      first_status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    return ESP_FAIL;
  return first_status;
}
extern "C" esp_err_t sentinel_arduino_nvs_status(void) { return first_status; }
