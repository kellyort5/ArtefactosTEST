#pragma once
#include "esp_err.h"
// Application adapter linked by the approved official-board menu option.
// Returns the actual first NVS result; never reports an error as healthy NVS.
extern "C" esp_err_t sentinel_arduino_nvs_status(void);
