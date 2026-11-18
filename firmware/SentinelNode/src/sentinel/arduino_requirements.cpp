#include "esp_arduino_version.h"
#include "sdkconfig.h"
// Pin actual distributed SDK headers. Application flags never rebuild IDF.
#if !CONFIG_IDF_TARGET_ESP32S3 || !CONFIG_ESPTOOLPY_FLASHSIZE_16MB ||          \
    !CONFIG_SPIRAM || !CONFIG_SPIRAM_MODE_OCT || !CONFIG_SPIRAM_SPEED_80M
#error Sentinel requires ESP32S3 Dev Module with 16MB flash and OPI PSRAM
#endif
#if ESP_ARDUINO_VERSION_MAJOR != 3 || ESP_ARDUINO_VERSION_MINOR != 3 ||        \
    ESP_ARDUINO_VERSION_PATCH != 12
#error Sentinel requires the verified official ESP32 Arduino core 3.3.12
#endif
#if !defined(ARDUINO_PARTITION_sentinel)
#error Select the approved Sentinel standard USB partition option before building
#endif
#if CONFIG_ESP_PHY_INIT_DATA_IN_PARTITION
#error Sentinel requires the official SDK with PHY init data embedded in the application
#endif
#if !CONFIG_BT_ENABLED || !CONFIG_BT_NIMBLE_ENABLED ||                         \
    !CONFIG_BT_NIMBLE_SM_SC || CONFIG_BT_NIMBLE_SM_SC_DEBUG_KEYS ||            \
    !CONFIG_BT_NIMBLE_NVS_PERSIST || CONFIG_BT_NIMBLE_MAX_BONDS != 3
#error Sentinel requires production SC-capable BLE and the verified persistent 3-bond store
#endif
// SC-only/MITM/one connection are enforced in the BLE adapter at runtime;
// compiled legacy support is never accepted as permission to use legacy links.
// HTTPS/DoH retain certificate validation; MQTT uses TCP in the LAN MVP.
#if !CONFIG_MBEDTLS_SSL_PROTO_TLS1_2 || CONFIG_ESP_TLS_INSECURE || \
    CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY
#error Sentinel requires verified HTTPS support for Sanitizador
#endif
#if !CONFIG_LWIP_IP_FORWARD || !CONFIG_LWIP_IPV4_NAPT
#error Sentinel requires official forwarding and NAPT resources for Sanitizador
#endif
