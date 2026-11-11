#include "network.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mbedtls/x509_crt.h"
#include "mqtt5_client.h"
#include "sentinel.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#if !CONFIG_MBEDTLS_HAVE_TIME_DATE || defined(CONFIG_ESP_TLS_INSECURE) ||      \
    defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
#error Sentinel requires TLS certificate date and server verification
#endif
namespace sentinel {
static char command_topic[160];
static std::atomic<esp_mqtt_client_handle_t> active_client{nullptr};
static std::atomic<bool> reported_online{false};
static void wifi_event(void *, esp_event_base_t base, int32_t id, void *) {
  if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    post({Kind::WifiUp});
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    post({Kind::WifiDown});
}
static void mqtt_event(void *, esp_event_base_t, int32_t id, void *data) {
  auto e = static_cast<esp_mqtt_event_handle_t>(data);
  if (e->client != active_client)
    return;
  static char rx[MaxMessage + 1];
  static size_t used = 0, total = 0;
  static bool retained = false;
  if (id == MQTT_EVENT_CONNECTED) {
    reported_online = true;
    post({Kind::MqttUp});
  }
  if (id == MQTT_EVENT_DISCONNECTED) {
    reported_online = false;
    wipe(rx, sizeof(rx));
    used = total = 0;
    post({Kind::MqttDown});
  }
  if (id == MQTT_EVENT_ERROR)
    post({Kind::MqttError});
  if (id != MQTT_EVENT_DATA)
    return;
  if (e->current_data_offset == 0) {
    wipe(rx, sizeof(rx));
    used = total = 0;
    if (e->topic_len != static_cast<int>(strlen(command_topic)) ||
        memcmp(e->topic, command_topic, e->topic_len) ||
        e->total_data_len <= 0 ||
        e->total_data_len > static_cast<int>(MaxMessage))
      return;
    total = e->total_data_len;
    retained = e->retain;
  }
  if (!total || e->current_data_offset != static_cast<int>(used) ||
      e->data_len < 0 || used + e->data_len > total) {
    wipe(rx, sizeof(rx));
    used = total = 0;
    return;
  }
  memcpy(rx + used, e->data, e->data_len);
  used += e->data_len;
  if (used == total) {
    rx[used] = 0;
    submit(Channel::Mqtt, rx, used, true, retained);
    wipe(rx, sizeof(rx));
    used = total = 0;
  }
}
bool Network::init(const char *id) {
  snprintf(device, sizeof(device), "%s", id);
  snprintf(client_id, sizeof(client_id), "node-%s", id);
  snprintf(prefix, sizeof(prefix), "sentinel/v1/nodes/%s/", id);
  snprintf(command_topic, sizeof(command_topic), "%scommands", prefix);
  snprintf(will_topic, sizeof(will_topic), "%savailability", prefix);
  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK ||
      !esp_netif_create_default_wifi_sta())
    return false;
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.nvs_enable = 0;
  if (esp_wifi_init(&cfg) != ESP_OK ||
      esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
      esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK)
    return false;
  if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event,
                                 nullptr) != ESP_OK ||
      esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event,
                                 nullptr) != ESP_OK)
    return false;
  // Application emits sanitized state/errors. Library diagnostics never print
  // provisioned material.
  esp_log_level_set("wifi", ESP_LOG_NONE);
  esp_log_level_set("mqtt_client", ESP_LOG_NONE);
  esp_log_level_set("esp-tls", ESP_LOG_NONE);
  esp_log_level_set("transport_base", ESP_LOG_NONE);
  initialized = true;
  return true;
}
const char *Network::configure(const cJSON *args) {
  mbedtls_x509_crt cert;
  mbedtls_x509_crt_init(&cert);
  const char *pem = str(args, "ca_certificate");
  int rc = mbedtls_x509_crt_parse(
      &cert, reinterpret_cast<const unsigned char *>(pem), strlen(pem) + 1);
  bool ca_valid = rc == 0 && cert.MBEDTLS_PRIVATE(ca_istrue);
  mbedtls_x509_crt_free(&cert);
  if (!ca_valid)
    return "invalid_message";
  bool restart = sta;
  if (!stop_network())
    return "resource_error";
  wipe(ssid, sizeof(ssid));
  wipe(wifi_password, sizeof(wifi_password));
  wipe(uri, sizeof(uri));
  wipe(username, sizeof(username));
  wipe(password, sizeof(password));
  wipe(ca, sizeof(ca));
  snprintf(ssid, sizeof(ssid), "%s", str(args, "ssid"));
  snprintf(wifi_password, sizeof(wifi_password), "%s",
           str(args, "wifi_password"));
  snprintf(uri, sizeof(uri), "%s", str(args, "mqtt_uri"));
  snprintf(username, sizeof(username), "%s", str(args, "mqtt_username"));
  snprintf(password, sizeof(password), "%s", str(args, "mqtt_password"));
  snprintf(ca, sizeof(ca), "%s", pem);
  configured = true;
  return restart && !start_cebo() ? "resource_error" : nullptr;
}
bool Network::stop_network() {
  online = false;
  ip = false;
  mqtt_state = "disconnected";
  wifi_state = "disconnected";
  if (client) {
    active_client = nullptr;
    reported_online = false;
    if (client_started) {
      esp_mqtt5_disconnect_property_config_t disconnect = {};
      disconnect.disconnect_reason = 4;
      disconnect.session_expiry_interval = 0;
      if (esp_mqtt5_client_set_disconnect_property(client, &disconnect) !=
          ESP_OK)
        return false;
      esp_err_t err = esp_mqtt_client_stop(client);
      if (err != ESP_OK)
        return false;
      client_started = false;
    }
    if (esp_mqtt_client_destroy(client) != ESP_OK)
      return false;
    client = nullptr;
  }
  if (sta) {
    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STARTED)
      return false;
    sta = false;
  }
  if (initialized && esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK)
    return false;
  next_wifi = next_mqtt = 0;
  wifi_backoff = mqtt_backoff = 1;
  return true;
}
bool Network::start_cebo() {
  if (!initialized || !network_released())
    return false;
  if (!configured) {
    wifi_state = "disconnected";
    mqtt_state = "disconnected";
    return true;
  }
  wifi_config_t cfg = {};
  memcpy(cfg.sta.ssid, ssid, strlen(ssid));
  memcpy(cfg.sta.password, wifi_password, strlen(wifi_password));
  cfg.sta.pmf_cfg.capable = true;
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err == ESP_OK)
    err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
  wipe(&cfg, sizeof(cfg));
  if (err != ESP_OK)
    return false;
  if (esp_wifi_start() != ESP_OK) {
    esp_wifi_set_mode(WIFI_MODE_NULL);
    return false;
  }
  sta = true;
  wifi_state = "connecting";
  next_wifi = monotonic_ms();
  return true;
}
void Network::wifi_up() {
  if (!sta)
    return;
  wifi_ap_record_t ap = {};
  if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK)
    return;
  ip = true;
  wifi_state = "connected";
  wifi_backoff = 1;
  next_wifi = 0;
  next_mqtt = monotonic_ms();
}
void Network::wifi_down() {
  if (!sta)
    return;
  ip = false;
  online = false;
  wifi_state = "error";
  mqtt_state = "disconnected";
  next_wifi = monotonic_ms() + wifi_backoff * 1000;
  wifi_backoff = wifi_backoff < 16 ? wifi_backoff * 2 : 30;
}
void Network::mqtt_up() {
  if (!sta || !ip || !client || !reported_online)
    return;
  online = true;
  mqtt_state = "connected";
  mqtt_backoff = 1;
  // Preserve RETAIN on forwarded publications; firmware rejects retained orders
  // even after subscription.
  esp_mqtt5_subscribe_property_config_t props = {};
  props.retain_as_published_flag = true;
  props.retain_handle = 0;
  if (esp_mqtt5_client_set_subscribe_property(client, &props) != ESP_OK ||
      esp_mqtt_client_subscribe(client, command_topic, 1) < 0) {
    reported_online = false;
    mqtt_down(true);
    return;
  }
  publish("availability", "online", 6, 1, true);
}
void Network::mqtt_down(bool error) {
  if (!client || !sta || reported_online)
    return;
  online = false;
  mqtt_state = error ? "error" : "disconnected";
  next_mqtt = monotonic_ms() + mqtt_backoff * 1000;
  mqtt_backoff = mqtt_backoff < 16 ? mqtt_backoff * 2 : 30;
}
void Network::tick(uint64_t now, bool time_valid) {
  if (!sta)
    return;
  if (client && reported_online && !online)
    mqtt_up();
  if (client && !reported_online && online)
    mqtt_down();
  if (!ip) {
    auto netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info = {};
    wifi_ap_record_t ap = {};
    if (netif && esp_wifi_sta_get_ap_info(&ap) == ESP_OK &&
        esp_netif_get_ip_info(netif, &info) == ESP_OK && info.ip.addr)
      wifi_up();
  }
  if (!ip && now >= next_wifi) {
    if (esp_wifi_connect() != ESP_OK)
      wifi_state = "error";
    next_wifi = now + wifi_backoff * 1000;
    wifi_backoff = wifi_backoff < 16 ? wifi_backoff * 2 : 30;
  }
  if (!ip || online)
    return;
  if (!time_valid) {
    mqtt_state = "error";
    return;
  }
  if (now < next_mqtt)
    return;
  if (!client) {
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = uri;
    cfg.broker.verification.certificate = ca;
    cfg.broker.verification.skip_cert_common_name_check = false;
    cfg.credentials.client_id = client_id;
    cfg.credentials.username = username;
    cfg.credentials.authentication.password = password;
    cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;
    cfg.session.disable_clean_session = false;
    cfg.session.keepalive = 15;
    cfg.session.last_will.topic = will_topic;
    cfg.session.last_will.msg = "offline";
    cfg.session.last_will.msg_len = 7;
    cfg.session.last_will.qos = 1;
    cfg.session.last_will.retain = true;
    cfg.network.disable_auto_reconnect = true;
    cfg.network.timeout_ms = 5000;
    cfg.buffer.size = 3072;
    cfg.buffer.out_size = 3072;
    cfg.task.stack_size = 8192;
    cfg.outbox.limit = 32768;
    client = esp_mqtt_client_init(&cfg);
    active_client = client;
    reported_online = false;
    if (!client) {
      mqtt_down(true);
      next_mqtt = now + 30000;
      return;
    }
    esp_mqtt5_connection_property_config_t props = {};
    props.session_expiry_interval = 0;
    if (esp_mqtt5_client_set_connect_property(client, &props) != ESP_OK ||
        esp_mqtt_client_register_event(
            client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
            mqtt_event, nullptr) != ESP_OK) {
      esp_mqtt_client_destroy(client);
      active_client = nullptr;
      client = nullptr;
      mqtt_state = "error";
      next_mqtt = now + 30000;
      return;
    }
  }
  esp_err_t err = client_started ? esp_mqtt_client_reconnect(client)
                                 : esp_mqtt_client_start(client);
  if (err == ESP_OK) {
    client_started = true;
    mqtt_state = "connecting";
  } else
    mqtt_state = "error";
  next_mqtt = now + mqtt_backoff * 1000;
  mqtt_backoff = mqtt_backoff < 16 ? mqtt_backoff * 2 : 30;
}
bool Network::publish(const char *suffix, const char *data, size_t n, int qos,
                      bool retain) {
  if (!online || !client || n > MaxMessage)
    return false;
  char topic[160];
  snprintf(topic, sizeof(topic), "%s%s", prefix, suffix);
  return esp_mqtt_client_enqueue(client, topic, data, n, qos, retain, true) >=
         0;
}
} // namespace sentinel
