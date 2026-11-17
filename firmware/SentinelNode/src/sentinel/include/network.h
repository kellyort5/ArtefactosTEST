#pragma once
#include "capture_ingress.h"
#include "connectivity_config.h"
#include "core.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_transport.h"
#include <atomic>
namespace sentinel {
class Network : public Resources, public ModuleResources {
  CaptureIngress ingress;
  esp_netif_t *sta_handle = nullptr, *ap_handle = nullptr;
  Profile owner = Profile::Idle;
  bool associating = false, ap_active = false, napt_active = false,
       promiscuous = false;
  QueueHandle_t radio_queue = nullptr;
  std::atomic<uint64_t> radio_dropped{0};
  MqttTransport transport;
  bool initialized = false, sta = false, ip = false, online = false,
       configured = false;
  uint64_t next_wifi = 0, next_mqtt = 0;
  unsigned wifi_backoff = 1, mqtt_backoff = 1;
  ConnectivityConfig config;
  bool persistence_error = false;
  char device[65] = {}, client_id[72] = {}, prefix[128] = {},
       will_topic[160] = {};

public:
  const char *wifi_state = "disconnected", *mqtt_state = "disconnected";
  bool init(const char *id);
  const char *configure(const cJSON *args);
  bool configuration_error() const { return persistence_error; }
  bool stop_network() override;
  bool network_released() const override;
  bool network_stopping() const override { return transport.stopping(); }
  bool start_cebo() override;
  bool start_profile(Profile) override;
  bool set_channel(uint8_t) override;
  bool scan_passive() override;
  size_t scan_results(ScanNetwork *, size_t) override;
  bool read_scan_results(ScanNetwork *, size_t, size_t &) override;
  bool capture(bool) override;
  uint32_t capture_generation() const override { return ingress.generation(); }
  uint64_t capture_dropped() const override { return ingress.dropped(); }
  uint16_t allowed_channels() const override;
  void frame(const uint8_t *, size_t, bool, uint8_t, int8_t);
  bool configure_upstream(const UpstreamConfig &) override;
  bool configure_ap(const ApConfig &) override;
  bool napt(bool) override;
  esp_netif_t *sta_netif() const override {
    return owner == Profile::Sanitizador ? sta_handle : nullptr;
  }
  esp_netif_t *ap_netif() const override {
    return owner == Profile::Sanitizador ? ap_handle : nullptr;
  }
  bool poll(RadioEvent &) override;
  uint64_t dropped() const override { return radio_dropped.load(); }
  void notify(const RadioEvent &);
  void tick(uint64_t now, bool time_valid);
  void wifi_up();
  void wifi_down();
  void mqtt_up();
  void mqtt_down(bool error = false);
  bool publish(const char *suffix, const char *bytes, size_t n, int qos,
               bool retained);
  bool connected() const { return online; }
  bool active() const { return sta; }
};
} // namespace sentinel
