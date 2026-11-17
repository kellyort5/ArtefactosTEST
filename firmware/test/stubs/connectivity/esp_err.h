#pragma once
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_NVS_NOT_FOUND = 0x1102,
              ESP_ERR_NVS_INVALID_LENGTH = 0x110c, ESP_ERR_INVALID_STATE = 0x103,
              ESP_ERR_WIFI_NOT_STARTED = 0x3002,
              ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED = 0x5003;
