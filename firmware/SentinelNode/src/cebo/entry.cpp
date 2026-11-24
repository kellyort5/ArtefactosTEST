// Punto de entrada del módulo Cebo para el registro de sentinel_api. El núcleo
// lo enlaza solo cuando SENTINEL_MODULE_CEBO=1 (optional_modules.cpp).
#include "include/cebo.hpp"
#include <esp_netif.h>

namespace {
class EspCebo final : public sentinel::cebo::CeboPlatform {
public:
  bool lan_ready() override {
    // sta_netif() de ModuleResources solo se concede a Sanitizador; en Cebo el
    // núcleo gestiona la STA y aquí solo se lee su estado IPv4.
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info = {};
    return netif && esp_netif_is_netif_up(netif) &&
           esp_netif_get_ip_info(netif, &info) == ESP_OK && info.ip.addr != 0;
  }
  sentinel::cebo::ServiceListener &http() override {
    return sentinel::cebo::esp_http_listener();
  }
  sentinel::cebo::ServiceListener &telnet() override {
    return sentinel::cebo::esp_telnet_listener();
  }
};
} // namespace

extern "C" sentinel::Module *sentinel_cebo_module() {
  static EspCebo platform;
  static sentinel::cebo::Cebo module(platform);
  return &module;
}
