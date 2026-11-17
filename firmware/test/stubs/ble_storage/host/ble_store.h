#pragma once
#include <cstdint>
#define CONFIG_BT_NIMBLE_MAX_BONDS 3
#define CONFIG_BT_NIMBLE_MAX_CCCDS 8
#define MYNEWT_VAL_BLE_STORE_MAX_CSFCS 3
#define MYNEWT_VAL(k) MYNEWT_VAL_##k
struct ble_store_value_sec { uint8_t key_size; unsigned authenticated:1; uint8_t sc:1; uint8_t opaque[64]; };
struct ble_store_value_cccd { uint8_t opaque[16]; };
struct ble_store_value_csfc { uint8_t opaque[16]; };
struct ble_store_value_local_irk { uint8_t opaque[24]; };
struct ble_store_value_rpa_rec { uint8_t opaque[24]; };
