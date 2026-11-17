#include "include/network.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "http_parser.h"
#include "include/sentinel.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
namespace sentinel {
static char command_topic[160];
static Network *radio_owner = nullptr;
static std::atomic<bool> reported_online{false}, reported_error{false};
static_assert(MqttTransport::MaxPayload == MaxMessage);
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
    if (id == WIFI_EVENT_SCAN_DONE) {
      auto done = static_cast<wifi_event_sta_scan_done_t *>(data);
      e.scan_status = done->status;
      e.scan_count = done->number;
    }
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
static void mqtt_status(bool connected, bool error) {
  reported_error = error;
  reported_online = connected;
  post({connected ? Kind::MqttUp : error ? Kind::MqttError : Kind::MqttDown});
}
static void mqtt_message(const char *data, size_t n, bool retained) {
  // The library preserves RETAIN on both initial and live publications.
  // The shared command validator/replay policy remains authoritative.
  submit(Channel::Mqtt, data, n, true, retained);
}
bool Network::init(const char *id) {
  if (initialized)
    return !strcmp(device, id);
  snprintf(device, sizeof(device), "%s", id);
  snprintf(client_id, sizeof(client_id), "node-%s", id);
  snprintf(prefix, sizeof(prefix), "sentinel/v2/nodes/%s/", id);
  snprintf(command_topic, sizeof(command_topic), "%scommands", prefix);
  snprintf(will_topic, sizeof(will_topic), "%savailability", prefix);
  radio_queue = xQueueCreate(RadioQueueSize, sizeof(RadioEvent));
  if (!radio_queue)
    return false;
  radio_owner = this;
  // Sentinel alone initializes netif/driver; Arduino WiFi.begin is never used.
  if (esp_netif_init() != ESP_OK)
    return false;
  // Arduino components may already own the default event loop. This is the
  // only documented already-initialized result we accept.
  esp_err_t event_status = esp_event_loop_create_default();
  if ((event_status != ESP_OK && event_status != ESP_ERR_INVALID_STATE) ||
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
  auto restored = load_connectivity(config);
  configured = restored == ConnectivityLoad::Loaded;
  persistence_error = restored == ConnectivityLoad::Error;
  // Recovery never starts STA, chooses a profile or makes old UTC trustworthy.
  initialized = true;
  return true;
}
const char *Network::configure(const cJSON *args) {
  ConnectivityConfig candidate;
  if (!candidate.from_args(args))
    return "invalid_message";
  if (owner != Profile::Idle && owner != Profile::Cebo)
    return "invalid_state";
  // A provision command is synchronous. Reject it without side effects while
  // the worker owns TLS; an explicit transition to Idle releases that session.
  // Never begin an asynchronous stop and then report an unrelated apply error.
  if (!transport.stopped())
    return "busy";
  // Do not touch the running radio or previous RAM config on a storage failure.
  if (!save_connectivity(candidate)) {
    persistence_error = true;
    return "resource_error";
  }
  persistence_error = false;
  bool restart = sta;
  auto saved_owner = owner;
  if (!stop_network())
    return "resource_error";
  wipe(&config, sizeof(config));
  config = candidate;
  configured = true;
  owner = saved_owner;
  return restart && !start_cebo() ? "resource_error" : nullptr;
}
bool Network::stop_network() {
  if (napt_active && !napt(false))
    return false;
  ingress.close();
  if (promiscuous && esp_wifi_set_promiscuous(false) != ESP_OK)
    return false;
  promiscuous = false;
  online = false;
  ip = false;
  mqtt_state = "disconnected";
  wifi_state = "disconnected";
  reported_online = false;
  transport.request_stop();
  if (!transport.stopped())
    return false;
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
  if (sta || !transport.stopped() || ap_active || napt_active || promiscuous)
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
  memcpy(cfg.sta.ssid, config.ssid, strlen(config.ssid));
  memcpy(cfg.sta.password, config.wifi_password, strlen(config.wifi_password));
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
  if (!sta || !ip || !transport.connected() || !reported_online)
    return;
  online = true;
  mqtt_state = "connected";
  mqtt_backoff = 1;
  // Subscription and broker ACK were completed by the I/O worker already.
  publish("availability", "online", 6, 1, true);
}
void Network::mqtt_down(bool error) {
  if (!sta || reported_online)
    return;
  online = false;
  mqtt_state = error ? "error" : "disconnected";
  next_mqtt = monotonic_ms() + mqtt_backoff * 1000;
  mqtt_backoff = mqtt_backoff < 16 ? mqtt_backoff * 2 : 30;
}
void Network::tick(uint64_t now, bool time_valid) {
  if (!associating)
    return;
  if (reported_online && !online)
    mqtt_up();
  if (!reported_online &&
      (online || (transport.stopped() && !strcmp(mqtt_state, "connecting"))))
    mqtt_down(reported_error);
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
  if (!transport.stopped())
    return;
  MqttTransport::Config cfg;
  http_parser_url parsed = {};
  if (http_parser_parse_url(config.uri, strlen(config.uri), 0, &parsed) != 0 ||
      !(parsed.field_set & (1 << UF_HOST)) ||
      parsed.field_data[UF_HOST].len >= sizeof(cfg.host)) {
    mqtt_down(true);
    return;
  }
  memcpy(cfg.host, config.uri + parsed.field_data[UF_HOST].off,
         parsed.field_data[UF_HOST].len);
  cfg.port = parsed.field_set & (1 << UF_PORT) ? parsed.port : 8883;
  cfg.time_valid = time_valid;
  snprintf(cfg.ca, sizeof(cfg.ca), "%s", config.ca);
  snprintf(cfg.user, sizeof(cfg.user), "%s", config.username);
  snprintf(cfg.password, sizeof(cfg.password), "%s", config.password);
  snprintf(cfg.client_id, sizeof(cfg.client_id), "%s", client_id);
  snprintf(cfg.commands, sizeof(cfg.commands), "%s", command_topic);
  snprintf(cfg.availability, sizeof(cfg.availability), "%s", will_topic);
  bool started =
      transport.init(mqtt_status, mqtt_message) && transport.start(cfg);
  wipe(&cfg, sizeof(cfg));
  mqtt_state = started ? "connecting" : "error";
  next_mqtt = now + mqtt_backoff * 1000;
  mqtt_backoff = mqtt_backoff < 16 ? mqtt_backoff * 2 : 30;
}
bool Network::publish(const char *suffix, const char *data, size_t n, int qos,
                      bool retain) {
  if (!online || !transport.connected() || n > MaxMessage)
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
  return transport.enqueue(topic, data, n, qos, retain);
}
} // namespace sentinel

namespace sentinel {
static void capture_frame(void *packet, wifi_promiscuous_pkt_type_t type) {
  if (!radio_owner || !packet || type != WIFI_PKT_MGMT)
    return;
  auto p = static_cast<wifi_promiscuous_pkt_t *>(packet);
  // sig_len includes the four-byte FCS (pinned ESP-IDF rx_ctrl definition).
  size_t size = p->rx_ctrl.sig_len;
  radio_owner->frame(p->payload, size >= 4 ? size - 4 : 0,
                     p->rx_ctrl.rx_state == 0, p->rx_ctrl.channel,
                     p->rx_ctrl.rssi);
}
void Network::frame(const uint8_t *bytes, size_t n, bool valid, uint8_t channel,
                    int8_t rssi) {
  RadioEvent e;
  if (ingress.copy(bytes, n, valid, channel, rssi, monotonic_ms(), e))
    notify(e);
}
void Network::notify(const RadioEvent &e) {
  if (e.kind == RadioEvent::Kind::Frame && !ingress.current(e.generation)) {
    ingress.drop();
    return;
  }
  if (!radio_queue || xQueueSend(radio_queue, &e, 0) != pdTRUE) {
    ++radio_dropped;
    if (e.kind == RadioEvent::Kind::Frame)
      ingress.drop();
  }
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
         (allowed_channels() & (1U << (channel - 1))) &&
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
  size_t count = 0;
  return read_scan_results(out, capacity, count) ? count : 0;
}
bool Network::read_scan_results(ScanNetwork *out, size_t capacity,
                                size_t &count) {
  count = 0;
  if (owner != Profile::Escudo || !out || !capacity || capacity > 32)
    return false;
  wifi_ap_record_t records[32] = {};
  uint16_t total = capacity;
  if (esp_wifi_scan_get_ap_records(&total, records) != ESP_OK) {
    esp_wifi_clear_ap_list();
    return false;
  }
  for (size_t i = 0; i < total; ++i) {
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
  count = total;
  return true;
}
uint16_t Network::allowed_channels() const {
  if (owner != Profile::Escudo || !sta)
    return 0;
  wifi_country_t country = {};
  if (esp_wifi_get_country(&country) != ESP_OK)
    return 0;
  uint16_t mask = 0;
  unsigned end = unsigned(country.schan) + country.nchan;
  for (unsigned c = country.schan; c < end && c <= 14; ++c)
    if (c)
      mask |= 1U << (c - 1);
  return mask;
}
bool Network::capture(bool enabled) {
  if (owner != Profile::Escudo || !sta)
    return false;
  if (enabled && promiscuous && ingress.generation())
    return true;
  ingress.close(); // copied frames of the prior epoch can no longer be admitted
  if (!enabled) {
    if (promiscuous && esp_wifi_set_promiscuous(false) != ESP_OK)
      return false;
    promiscuous = false;
    return true;
  }
  wifi_promiscuous_filter_t filter = {};
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  if (esp_wifi_set_promiscuous_rx_cb(capture_frame) != ESP_OK ||
      esp_wifi_set_promiscuous_filter(&filter) != ESP_OK)
    return false;
  ingress.open();
  if (esp_wifi_set_promiscuous(true) != ESP_OK) {
    ingress.close();
    return false;
  }
  promiscuous = true;
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
