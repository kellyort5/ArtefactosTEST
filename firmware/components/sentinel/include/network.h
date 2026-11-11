#pragma once
#include "core.h"
#include "mqtt_client.h"
namespace sentinel {
class Network : public Resources {
  esp_mqtt_client_handle_t client = nullptr;
  bool initialized = false, sta = false, ip = false, online = false,
       configured = false, client_started = false;
  uint64_t next_wifi = 0, next_mqtt = 0;
  unsigned wifi_backoff = 1, mqtt_backoff = 1;
  char ssid[33] = {}, wifi_password[64] = {}, uri[257] = {}, username[257] = {},
       password[513] = {}, ca[2049] = {};
  char device[65] = {}, client_id[72] = {}, prefix[128] = {},
       will_topic[160] = {};

public:
  const char *wifi_state = "disconnected", *mqtt_state = "disconnected";
  bool init(const char *id);
  const char *configure(const cJSON *args);
  bool stop_network() override;
  bool network_released() const override { return !sta && !client; }
  bool start_cebo() override;
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
