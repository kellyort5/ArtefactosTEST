#include "include/mqtt_transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "include/mqtt_dns.h"
#include "include/transport_requirements.h"
#include <Arduino.h>
#include <NetworkClient.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <new>
namespace sentinel {
namespace {
void clear_bytes(void *p, size_t n) {
  auto bytes = static_cast<volatile uint8_t *>(p);
  while (n--)
    *bytes++ = 0;
}
} // namespace
struct MqttTransport::Impl {
  struct Tx {
    char topic[160];
    uint8_t payload[MaxPayload];
    uint16_t size;
    uint8_t qos;
    bool retained;
  };
  static_assert(sizeof(Tx) * 14 <= 32768, "bounded MQTT outbox");
  Config config;
  StateCallback state = nullptr;
  MessageCallback message = nullptr;
  QueueHandle_t queue = nullptr;
  std::atomic<bool> busy{false}, cancel{false}, online{false};
  NetworkClient socket;
  MqttDns dns;
  MqttClient mqtt = {};
  MqttNet net = {};
  uint8_t tx_buffer[3072] = {}, rx_buffer[3072] = {};
  char incoming[MaxPayload + 1] = {};
  size_t received = 0, expected = 0;
  bool wanted = false, retained = false, has_pending = false, closing = false;
  Tx pending = {};
  uint16_t packet_id = 0;

  static int connect(void *arg, const char *host, word16 port, int) {
    auto &s = *static_cast<Impl *>(arg);
    IPAddress ip;
    if (!s.dns.resolve(host, ip, s.cancel) || s.cancel)
      return MQTT_CODE_ERROR_NETWORK;
    s.socket.setTimeout(1);
    // Official Arduino TCP client; resolve once and bound connect to one second.
    if (!s.socket.connect(ip, port, 1000) || s.cancel) {
      s.socket.stop();
      return MQTT_CODE_ERROR_NETWORK;
    }
    return MQTT_CODE_SUCCESS;
  }
  static int read(void *arg, byte *buf, int n, int timeout) {
    auto &s = *static_cast<Impl *>(arg);
    uint32_t end = millis() + std::min(std::max(timeout, 1), 1000);
    while (!s.cancel) {
      int available = s.socket.available();
      if (available > 0)
        return s.socket.read(buf, std::min(n, available));
      if (!s.socket.connected())
        return MQTT_CODE_ERROR_NETWORK;
      if (static_cast<int32_t>(end - millis()) <= 0)
        return MQTT_CODE_ERROR_TIMEOUT;
      delay(1);
    }
    return MQTT_CODE_ERROR_NETWORK;
  }
  static int write(void *arg, const byte *buf, int n, int) {
    auto &s = *static_cast<Impl *>(arg);
    if ((s.cancel && !s.closing) || !s.socket.connected())
      return MQTT_CODE_ERROR_NETWORK;
    int sent = s.socket.write(buf, n);
    return sent > 0 ? sent : MQTT_CODE_ERROR_NETWORK;
  }
  static int disconnect(void *arg) {
    static_cast<Impl *>(arg)->socket.stop();
    return MQTT_CODE_SUCCESS;
  }
  void reset_incoming() {
    clear_bytes(incoming, sizeof(incoming));
    received = expected = 0;
    wanted = retained = false;
  }
  static int receive(MqttClient *client, MqttMessage *msg, byte first,
                     byte done) {
    auto &s = *static_cast<Impl *>(client->ctx);
    if (first) {
      s.reset_incoming();
      size_t topic_length = strlen(s.config.commands);
      s.wanted = msg->topic_name_len == topic_length &&
                 !memcmp(msg->topic_name, s.config.commands, topic_length) &&
                 msg->total_len > 0 && msg->total_len <= MaxPayload;
      if (s.wanted) {
        s.expected = msg->total_len;
        s.retained = msg->retain;
      }
    }
    if (s.wanted) {
      if (msg->buffer_pos != s.received ||
          msg->buffer_len > s.expected - s.received)
        s.wanted = false;
      else {
        memcpy(s.incoming + s.received, msg->buffer, msg->buffer_len);
        s.received += msg->buffer_len;
      }
    }
    if (done) {
      if (s.wanted && s.received == s.expected && !s.cancel && s.message)
        s.message(s.incoming, s.received, s.retained);
      s.reset_incoming();
    }
    return MQTT_CODE_SUCCESS;
  }
  uint16_t next_id() {
    if (++packet_id == 0)
      ++packet_id;
    return packet_id;
  }
  int establish() {
    net.context = this;
    net.connect = connect;
    net.read = read;
    net.write = write;
    net.disconnect = disconnect;
    int rc = MqttClient_Init(&mqtt, &net, receive, tx_buffer, sizeof(tx_buffer),
                             rx_buffer, sizeof(rx_buffer), 1000);
    if (rc != MQTT_CODE_SUCCESS)
      return rc;
    mqtt.ctx = this;
    // use_tls=0 selects the official TCP client for the trusted-LAN MVP.
    rc = MqttClient_NetConnect(&mqtt, config.host, config.port, 1000, 0,
                               nullptr);
    if (rc != MQTT_CODE_SUCCESS || cancel)
      return MQTT_CODE_ERROR_NETWORK;
    MqttProp expiry = {};
    expiry.type = MQTT_PROP_SESSION_EXPIRY_INTERVAL;
    expiry.data_int = 0;
    MqttMessage will = {};
    will.topic_name = config.availability;
    will.buffer = reinterpret_cast<byte *>(const_cast<char *>("offline"));
    will.total_len = 7;
    will.qos = MQTT_QOS_1;
    will.retain = 1;
    MqttConnect c = {};
    c.protocol_level = 5;
    c.keep_alive_sec = 15;
    c.clean_session = 1;
    c.client_id = config.client_id;
    // Anonymous MQTT CONNECT: zero-initialized username/password pointers.
    c.enable_lwt = 1;
    c.lwt_msg = &will;
    c.props = &expiry;
    rc = MqttClient_Connect(&mqtt, &c);
    if (rc != MQTT_CODE_SUCCESS || cancel)
      return MQTT_CODE_ERROR_NETWORK;
    MqttTopic topic = {};
    topic.topic_filter = config.commands;
    topic.qos = MQTT_QOS_1;
    topic.sub_options =
        MQTT_SUBSCRIBE_RETAIN_AS_PUBLISHED | MQTT_SUBSCRIBE_RETAIN_HANDLING_0;
    MqttSubscribe sub = {};
    sub.protocol_level = 5;
    sub.packet_id = next_id();
    sub.topic_count = 1;
    sub.topics = &topic;
    rc = MqttClient_Subscribe(&mqtt, &sub);
    if (rc == MQTT_CODE_SUCCESS &&
        (sub.ack.return_code_count != 1 || sub.ack.return_codes[0] > 1))
      rc = MQTT_CODE_ERROR_NETWORK;
    return rc;
  }
  int publish_pending() {
    MqttPublish p = {};
    p.protocol_level = 5;
    p.packet_id = pending.qos ? next_id() : 0;
    p.qos = static_cast<MqttQoS>(pending.qos);
    p.retain = pending.retained;
    p.topic_name = pending.topic;
    p.buffer = pending.payload;
    p.total_len = p.buffer_len = pending.size;
    int rc = MqttClient_Publish(&mqtt, &p);
    if (rc == MQTT_CODE_SUCCESS) {
      has_pending = false;
      clear_bytes(&pending, sizeof(pending));
    }
    return rc;
  }
  void run() {
    int rc = establish();
    if (rc == MQTT_CODE_SUCCESS && !cancel) {
      online = true;
      if (state)
        state(true, false);
      while (!cancel) {
        if (!has_pending && xQueueReceive(queue, &pending, 0) == pdTRUE)
          has_pending = true;
        rc =
            has_pending ? publish_pending() : MqttClient_WaitMessage(&mqtt, 50);
        if (rc == MQTT_CODE_ERROR_TIMEOUT && !has_pending)
          continue;
        if (rc != MQTT_CODE_SUCCESS)
          break;
      }
    }
    online = false;
    if (cancel && socket.connected()) {
      closing = true;
      MqttProp expiry = {};
      expiry.type = MQTT_PROP_SESSION_EXPIRY_INTERVAL;
      MqttDisconnect d = {};
      d.protocol_level = 5;
      d.reason_code = 4; // Preserve the broker's offline last-will publication.
      d.props = &expiry;
      MqttClient_Disconnect_ex(&mqtt, &d);
      closing = false;
    }
    MqttClient_NetDisconnect(&mqtt);
    MqttClient_DeInit(&mqtt);
    reset_incoming();
    clear_bytes(tx_buffer, sizeof(tx_buffer));
    clear_bytes(rx_buffer, sizeof(rx_buffer));
    clear_bytes(&config, sizeof(config));
    if (cancel) {
      // Explicit mode release discards the old transport session. Reconnects
      // keep queued/pending payloads with their original application
      // identities.
      xQueueReset(queue);
      has_pending = false;
      clear_bytes(&pending, sizeof(pending));
    }
    if (state)
      state(false, rc != MQTT_CODE_SUCCESS && !cancel);
    busy = false;
    vTaskDelete(nullptr); // No access to this object after releasing busy.
  }
  static void worker(void *arg) { static_cast<Impl *>(arg)->run(); }
};
bool MqttTransport::init(StateCallback state, MessageCallback message) {
  if (impl)
    return true;
  auto p = new (std::nothrow) Impl;
  if (!p)
    return false;
  p->queue = xQueueCreate(14, sizeof(Impl::Tx));
  if (!p->queue) {
    delete p;
    return false;
  }
  p->state = state;
  p->message = message;
  impl = p;
  return true;
}
bool MqttTransport::start(const Config &config) {
  if (!impl || !config.host[0] || !config.port)
    return false;
  bool idle = false;
  if (!impl->busy.compare_exchange_strong(idle, true))
    return false;
  impl->config = config;
  impl->cancel = false;
  if (xTaskCreate(Impl::worker, "mqtt_io", 8192, impl, 4, nullptr) != pdPASS) {
    clear_bytes(&impl->config, sizeof(impl->config));
    impl->busy = false;
    return false;
  }
  return true;
}
void MqttTransport::request_stop() {
  if (!impl)
    return;
  impl->cancel = true;
  if (!impl->busy) {
    xQueueReset(impl->queue);
    impl->has_pending = false;
    clear_bytes(&impl->pending, sizeof(impl->pending));
  }
}
bool MqttTransport::stopped() const { return !impl || !impl->busy; }
bool MqttTransport::stopping() const {
  return impl && impl->cancel && impl->busy;
}
bool MqttTransport::connected() const { return impl && impl->online; }
bool MqttTransport::enqueue(const char *topic, const char *data, size_t n,
                            int qos, bool retain) {
  if (!connected() || impl->cancel || !topic || strlen(topic) >= 160 ||
      n > MaxPayload || (n && !data) || qos < 0 || qos > 2)
    return false;
  Impl::Tx tx = {};
  memcpy(tx.topic, topic, strlen(topic) + 1);
  if (n)
    memcpy(tx.payload, data, n);
  tx.size = n;
  tx.qos = qos;
  tx.retained = retain;
  bool ok = xQueueSend(impl->queue, &tx, 0) == pdTRUE;
  clear_bytes(&tx, sizeof(tx));
  return ok;
}
} // namespace sentinel
