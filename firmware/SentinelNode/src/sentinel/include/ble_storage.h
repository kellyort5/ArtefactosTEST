#pragma once
#include "esp_err.h"
namespace sentinel {
// Read-only compatibility check before NimBLE can restore or write bonds.
// Incompatible capacity/format is preserved for an explicit recovery decision.
esp_err_t ble_storage_preflight();
} // namespace sentinel
