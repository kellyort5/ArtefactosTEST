#pragma once
#include "esp_err.h"
#include <cstddef>
#include <cstdint>
using esp_event_base_t = const char *;
extern esp_event_base_t WIFI_EVENT, IP_EVENT;
constexpr int WIFI_EVENT_SCAN_DONE = 1, WIFI_EVENT_STA_CONNECTED = 2,
              WIFI_EVENT_STA_DISCONNECTED = 3, WIFI_EVENT_AP_STACONNECTED = 4,
              WIFI_EVENT_AP_STADISCONNECTED = 5, IP_EVENT_STA_GOT_IP = 6,
              ESP_EVENT_ANY_ID = -1;
struct esp_netif_obj {};
using esp_netif_t = esp_netif_obj;
struct ip4_addr_t { uint32_t addr = 0; };
struct esp_netif_ip_info_t { ip4_addr_t ip, netmask, gw; };
struct ip_event_got_ip_t { esp_netif_ip_info_t ip_info; };
struct wifi_event_sta_scan_done_t { uint32_t status; uint16_t number; };
struct wifi_event_ap_staconnected_t { uint8_t mac[6]; };
using wifi_event_ap_stadisconnected_t = wifi_event_ap_staconnected_t;
enum wifi_mode_t { WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_APSTA };
enum esp_netif_dhcp_status_t { ESP_NETIF_DHCP_STOPPED, ESP_NETIF_DHCP_STARTED };
constexpr int WIFI_STORAGE_RAM = 1, WIFI_IF_STA = 0, WIFI_IF_AP = 1,
              WIFI_AUTH_WPA2_PSK = 3, WIFI_SECOND_CHAN_NONE = 0,
              WIFI_SCAN_TYPE_PASSIVE = 1, WIFI_PROMIS_FILTER_MASK_MGMT = 1;
struct wifi_init_config_t { int nvs_enable = 1; };
#define WIFI_INIT_CONFIG_DEFAULT() wifi_init_config_t{}
struct wifi_config_t {
  struct { uint8_t ssid[32] = {}, password[64] = {};
    struct { bool capable = false; } pmf_cfg; } sta;
  struct { uint8_t ssid[32] = {}, password[64] = {}; unsigned ssid_len = 0;
    int authmode = 0, max_connection = 0, channel = 0;
    struct { bool capable = false; } pmf_cfg; } ap;
};
struct wifi_ap_record_t {
  uint8_t ssid[33] = {}, bssid[6] = {}, primary = 1;
  int8_t rssi = -50;
  int authmode = 0, pairwise_cipher = 0, group_cipher = 0;
};
struct wifi_scan_config_t { int scan_type; bool show_hidden;
  struct { unsigned passive; } scan_time; };
struct wifi_country_t { unsigned schan = 1, nchan = 11; };
enum wifi_promiscuous_pkt_type_t { WIFI_PKT_MGMT };
struct wifi_promiscuous_pkt_t {
  struct { unsigned sig_len; int rx_state; uint8_t channel; int8_t rssi; } rx_ctrl;
  uint8_t payload[1];
};
struct wifi_promiscuous_filter_t { unsigned filter_mask; };
using EventHandler = void (*)(void *, esp_event_base_t, int32_t, void *);
int esp_netif_init();
int esp_event_loop_create_default();
esp_netif_t *esp_netif_create_default_wifi_sta();
esp_netif_t *esp_netif_create_default_wifi_ap();
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *);
int esp_event_handler_register(esp_event_base_t, int, EventHandler, void *);
int esp_wifi_init(const wifi_init_config_t *);
int esp_wifi_set_storage(int);
int esp_wifi_set_mode(wifi_mode_t);
int esp_wifi_get_mode(wifi_mode_t *);
int esp_wifi_set_config(int, wifi_config_t *);
int esp_wifi_start();
int esp_wifi_stop();
int esp_wifi_connect();
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
int esp_wifi_set_promiscuous(bool);
int esp_wifi_set_channel(uint8_t, int);
int esp_wifi_scan_start(wifi_scan_config_t *, bool);
int esp_wifi_scan_get_ap_records(uint16_t *, wifi_ap_record_t *);
int esp_wifi_clear_ap_list();
int esp_wifi_get_country(wifi_country_t *);
int esp_wifi_set_promiscuous_rx_cb(void (*)(void *, wifi_promiscuous_pkt_type_t));
int esp_wifi_set_promiscuous_filter(wifi_promiscuous_filter_t *);
int esp_netif_dhcps_stop(esp_netif_t *);
int esp_netif_dhcps_start(esp_netif_t *);
int esp_netif_dhcps_get_status(esp_netif_t *, esp_netif_dhcp_status_t *);
int esp_netif_get_ip_info(esp_netif_t *, esp_netif_ip_info_t *);
int esp_netif_set_ip_info(esp_netif_t *, esp_netif_ip_info_t *);
int esp_netif_napt_enable(esp_netif_t *);
int esp_netif_napt_disable(esp_netif_t *);
constexpr int ESP_LOG_NONE = 0;
inline void esp_log_level_set(const char *, int) {}
