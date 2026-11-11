#include "network.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mbedtls/x509_crt.h"
#include "mqtt5_client.h"
#include "sentinel.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#if !CONFIG_MBEDTLS_HAVE_TIME_DATE || defined(CONFIG_ESP_TLS_INSECURE) ||      \
    defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
#error Sentinel requires TLS certificate date and server verification
#endif
namespace sentinel {
static char command_topic[160];
static Network *radio_owner = nullptr;
static std::atomic<esp_mqtt_client_handle_t> active_client{nullptr};
static std::atomic<bool> reported_online{false};
static void wifi_event(void *, esp_event_base_t base, int32_t id, void *data) {
  if (radio_owner &&
      (id == WIFI_EVENT_SCAN_DONE || id == WIFI_EVENT_STA_CONNECTED ||
       id == WIFI_EVENT_STA_DISCONNECTED) &&
      base == WIFI_EVENT) {
    RadioEvent e = {};
    e.uptime_ms = monotonic_ms();
    e.kind = id == WIFI_EVENT_SCAN_DONE       ? RadioEvent::Kind::ScanComplete
             : id == WIFI_EVENT_STA_CONNECTED ? RadioEvent::Kind::Connected
                                              : RadioEvent::Kind::Disconnected;
    radio_owner->notify(e);
  }
  if (base == WIFI_EVENT && radio_owner &&
      (id == WIFI_EVENT_AP_STACONNECTED ||
       id == WIFI_EVENT_AP_STADISCONNECTED)) {
    RadioEvent e = {};
    e.uptime_ms = monotonic_ms();
    e.kind = id == WIFI_EVENT_AP_STACONNECTED
                 ? RadioEvent::Kind::ApClientConnected
                 : RadioEvent::Kind::ApClientDisconnected;
    if (id == WIFI_EVENT_AP_STACONNECTED)
      memcpy(e.peer, static_cast<wifi_event_ap_staconnected_t *>(data)->mac, 6);
    else
      memcpy(e.peer, static_cast<wifi_event_ap_stadisconnected_t *>(data)->mac,
             6);
    radio_owner->notify(e);
  }
  if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    if (radio_owner) {
      auto ip = static_cast<ip_event_got_ip_t *>(data);
      RadioEvent e = {};
      e.kind = RadioEvent::Kind::GotIp;
      e.uptime_ms = monotonic_ms();
      e.ipv4 = ip->ip_info.ip.addr;
      e.netmask = ip->ip_info.netmask.addr;
      e.gateway = ip->ip_info.gw.addr;
      radio_owner->notify(e);
    }
    post({Kind::WifiUp});
  }
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
  snprintf(prefix, sizeof(prefix), "sentinel/v2/nodes/%s/", id);
  snprintf(command_topic, sizeof(command_topic), "%scommands", prefix);
  snprintf(will_topic, sizeof(will_topic), "%savailability", prefix);
  radio_queue = xQueueCreate(RadioQueueSize, sizeof(RadioEvent));
  if (!radio_queue)
    return false;
  radio_owner = this;
  if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK ||
      !(sta_handle = esp_netif_create_default_wifi_sta()) ||
      !(ap_handle = esp_netif_create_default_wifi_ap()))
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
  if (owner != Profile::Idle && owner != Profile::Cebo)
    return "invalid_state";
  bool restart = sta;
  auto saved_owner = owner;
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
  owner = saved_owner;
  return restart && !start_cebo() ? "resource_error" : nullptr;
}
bool Network::stop_network() {
  if (napt_active && !napt(false))
    return false;
  if (promiscuous && esp_wifi_set_promiscuous(false) != ESP_OK)
    return false;
  promiscuous = false;
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
  if (ap_handle) {
    auto err = esp_netif_dhcps_stop(ap_handle);
    if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
      return false;
  }
  associating = ap_active = false;
  owner = Profile::Idle;
  if (radio_queue)
    xQueueReset(radio_queue);
  if (initialized && esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK)
    return false;
  next_wifi = next_mqtt = 0;
  wifi_backoff = mqtt_backoff = 1;
  return true;
}
bool Network::network_released() const {
  if (sta || client || ap_active || napt_active || promiscuous)
    return false;
  if (!initialized)
    return true;
  wifi_mode_t mode;
  esp_netif_dhcp_status_t dhcp;
  return esp_wifi_get_mode(&mode) == ESP_OK && mode == WIFI_MODE_NULL &&
         esp_netif_dhcps_get_status(ap_handle, &dhcp) == ESP_OK &&
         dhcp != ESP_NETIF_DHCP_STARTED;
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
  sta = associating = true;
  wifi_state = "connecting";
  next_wifi = monotonic_ms();
  return true;
}
void Network::wifi_up() {
  if (!associating)
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
  if (!associating)
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
  if (!associating)
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
  if (!ip || online || owner != Profile::Cebo || !configured)
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
  if (!strcmp(suffix, "events") && n > 0) {
    auto j = cJSON_ParseWithLength(data, n);
    unsigned version = number(j, "version");
    cJSON_Delete(j);
    snprintf(topic, sizeof(topic), "sentinel/v%u/nodes/%s/%s",
             version == 1 ? 1 : 2, device, suffix);
  } else
    snprintf(topic, sizeof(topic), "%s%s", prefix, suffix);
  return esp_mqtt_client_enqueue(client, topic, data, n, qos, retain, true) >=
         0;
}
} // namespace sentinel

namespace sentinel {
static void capture_frame(void *packet, wifi_promiscuous_pkt_type_t type) {
  if (!radio_owner || type == WIFI_PKT_MISC)
    return;
  auto p = static_cast<wifi_promiscuous_pkt_t *>(packet);
  RadioEvent e = {};
  e.kind = RadioEvent::Kind::Frame;
  e.uptime_ms = monotonic_ms();
  e.channel = p->rx_ctrl.channel;
  e.rssi = p->rx_ctrl.rssi;
  e.original_length = p->rx_ctrl.sig_len;
  e.length = std::min<size_t>(e.original_length, sizeof(e.bytes));
  memcpy(e.bytes, p->payload, e.length);
  radio_owner->notify(e);
}
void Network::notify(const RadioEvent &e) {
  if (!radio_queue || xQueueSend(radio_queue, &e, 0) != pdTRUE)
    ++radio_dropped;
}
bool Network::poll(RadioEvent &e) {
  return radio_queue && xQueueReceive(radio_queue, &e, 0) == pdTRUE;
}
bool Network::start_profile(Profile p) {
  if (!initialized || !network_released())
    return false;
  owner = p;
  if (p == Profile::Idle)
    return true;
  if (p == Profile::Cebo)
    return start_cebo();
  if (p == Profile::Sanitizador)
    return true; // module requests upstream/AP explicitly
  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_start() != ESP_OK)
    return false;
  sta = true;
  associating = false;
  return true; // Escudo: never connect()
}
bool Network::set_channel(uint8_t channel) {
  return owner == Profile::Escudo && sta && channel >= 1 && channel <= 14 &&
         esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) == ESP_OK;
}
bool Network::scan_passive() {
  if (owner != Profile::Escudo || !sta)
    return false;
  wifi_scan_config_t config = {};
  config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
  config.show_hidden = true;
  config.scan_time.passive = 120;
  return esp_wifi_scan_start(&config, false) == ESP_OK;
}
size_t Network::scan_results(ScanNetwork *out, size_t capacity) {
  if (owner != Profile::Escudo || !out || !capacity || capacity > 32)
    return 0;
  wifi_ap_record_t records[32] = {};
  uint16_t count = capacity;
  if (esp_wifi_scan_get_ap_records(&count, records) != ESP_OK)
    return 0;
  for (size_t i = 0; i < count; ++i) {
    auto &a = out[i];
    auto &b = records[i];
    a = {};
    memcpy(a.ssid, b.ssid, 32);
    memcpy(a.bssid, b.bssid, 6);
    a.channel = b.primary;
    a.rssi = b.rssi;
    a.auth = b.authmode;
    a.pairwise_cipher = b.pairwise_cipher;
    a.group_cipher = b.group_cipher;
  }
  return count;
}
bool Network::capture(bool enabled) {
  if (owner != Profile::Escudo || !sta)
    return false;
  if (esp_wifi_set_promiscuous_rx_cb(capture_frame) != ESP_OK ||
      esp_wifi_set_promiscuous(enabled) != ESP_OK)
    return false;
  promiscuous = enabled;
  return true;
}
bool Network::configure_upstream(const UpstreamConfig &cfg) {
  if (owner != Profile::Sanitizador ||
      strnlen(cfg.ssid, sizeof(cfg.ssid)) > 32 || !*cfg.ssid ||
      strnlen(cfg.password, sizeof(cfg.password)) > 63)
    return false;
  wifi_config_t native = {};
  memcpy(native.sta.ssid, cfg.ssid, strlen(cfg.ssid));
  memcpy(native.sta.password, cfg.password, strlen(cfg.password));
  native.sta.pmf_cfg.capable = true;
  bool ok = esp_wifi_set_mode(ap_active ? WIFI_MODE_APSTA : WIFI_MODE_STA) ==
                ESP_OK &&
            esp_wifi_set_config(WIFI_IF_STA, &native) == ESP_OK;
  wipe(&native, sizeof(native));
  if (!ok)
    return false;
  if (!sta && esp_wifi_start() != ESP_OK)
    return false;
  sta = associating = true;
  ip = false;
  wifi_state = "connecting";
  next_wifi = monotonic_ms();
  return true;
}
bool Network::configure_ap(const ApConfig &cfg) {
  size_t key = strnlen(cfg.password, sizeof(cfg.password));
  if (owner != Profile::Sanitizador || !*cfg.ssid ||
      strnlen(cfg.ssid, sizeof(cfg.ssid)) > 32 || key < 8 || key > 63 ||
      !cfg.ipv4 || !cfg.netmask)
    return false;
  if (napt_active && !napt(false))
    return false;
  wifi_config_t native = {};
  memcpy(native.ap.ssid, cfg.ssid, strlen(cfg.ssid));
  native.ap.ssid_len = strlen(cfg.ssid);
  memcpy(native.ap.password, cfg.password, key);
  native.ap.authmode = WIFI_AUTH_WPA2_PSK;
  native.ap.max_connection = 4;
  native.ap.channel = 1;
  native.ap.pmf_cfg.capable = true;
  auto err = esp_netif_dhcps_stop(ap_handle);
  bool ok = (err == ESP_OK || err == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED);
  esp_netif_ip_info_t info = {};
  info.ip.addr = cfg.ipv4;
  info.gw.addr = cfg.ipv4;
  info.netmask.addr = cfg.netmask;
  ok = ok && esp_netif_set_ip_info(ap_handle, &info) == ESP_OK &&
       esp_netif_dhcps_start(ap_handle) == ESP_OK &&
       esp_wifi_set_mode(WIFI_MODE_APSTA) == ESP_OK &&
       esp_wifi_set_config(WIFI_IF_AP, &native) == ESP_OK;
  wipe(&native, sizeof(native));
  if (!ok)
    return false;
  if (!sta && esp_wifi_start() != ESP_OK)
    return false;
  sta = ap_active = true;
  return true;
}
bool Network::napt(bool enabled) {
  if (enabled && (owner != Profile::Sanitizador || !ap_active || !ip))
    return false;
  if (!enabled && !napt_active)
    return true;
  if ((enabled ? esp_netif_napt_enable(ap_handle)
               : esp_netif_napt_disable(ap_handle)) != ESP_OK)
    return false;
  napt_active = enabled;
  return true;
}
} // namespace sentinel
