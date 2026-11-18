#include "fake_runtime.h"
#include "Arduino.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <fstream>
FakeState server;
std::vector<std::thread> workers;
auto started = std::chrono::steady_clock::now();
uint32_t millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - started)
      .count();
}
void delay(unsigned ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
struct FakeQueue {
  std::mutex lock;
  std::deque<std::vector<unsigned char>> items;
  size_t count, size;
};
QueueHandle_t xQueueCreate(unsigned count, unsigned size) {
  auto *q = new FakeQueue;
  q->count = count;
  q->size = size;
  return q;
}
int xQueueSend(QueueHandle_t q, const void *p, unsigned) {
  std::lock_guard<std::mutex> l(q->lock);
  if (q->items.size() == q->count)
    return 0;
  q->items.emplace_back((const unsigned char *)p,
                        (const unsigned char *)p + q->size);
  return 1;
}
int xQueueReceive(QueueHandle_t q, void *p, unsigned) {
  std::lock_guard<std::mutex> l(q->lock);
  if (q->items.empty())
    return 0;
  memcpy(p, q->items.front().data(), q->size);
  q->items.pop_front();
  return 1;
}
void xQueueReset(QueueHandle_t q) {
  std::lock_guard<std::mutex> l(q->lock);
  q->items.clear();
}
void vQueueDelete(QueueHandle_t q) { delete q; }
int xTaskCreate(void (*fn)(void *), const char *, unsigned stack, void *ctx,
                unsigned, void *) {
  assert(stack == 8192);
  workers.emplace_back([=] { fn(ctx); });
  return 1;
}
void vTaskDelete(void *) {}
void join_workers() {
  for (auto &t : workers)
    if (t.joinable())
      t.join();
  workers.clear();
}
void reset_peer() {
  server.incoming.clear(); server.sent.clear(); server.closes = 0;
  server.entered = false; server.actual_ip.clear(); server.fail_connect = false;
}
void NetworkClient::setTimeout(unsigned s) { assert(s == 1); }
int NetworkClient::connect(IPAddress ip, uint16_t port, int32_t timeout) {
  assert(port == 1883 && timeout == 1000);
  server.entered = true;
  while (server.gate) delay(1);
  server.actual_ip = ip.value;
  live = !server.fail_connect;
  return live;
}
int NetworkClient::available() {
  std::lock_guard<std::mutex> l(server.lock);
  return server.incoming.size();
}
int NetworkClient::read(uint8_t *out, size_t n) {
  std::lock_guard<std::mutex> l(server.lock);
  n = std::min({n, server.incoming.size(), size_t(3)});
  for (size_t i = 0; i < n; i++) {
    out[i] = server.incoming.front();
    server.incoming.pop_front();
  }
  return n;
}
static void append(const std::vector<unsigned char> &v) {
  for (auto x : v)
    server.incoming.push_back(x);
}
size_t NetworkClient::write(const uint8_t *p, size_t n) {
  assert(live);
  std::lock_guard<std::mutex> l(server.lock);
  server.sent.emplace_back(p, p + n);
  unsigned type = p[0] >> 4;
  size_t pos = 1;
  while (p[pos++] & 128) {
  };
  if (type == 1)
    append({0x20, 3, 0, 0, 0});
  if (type == 8) {
    append({0x90, 4, p[pos], p[pos + 1], 0, 1});
    if (server.inject_retained)
      append(server.retained_packet);
  }
  if (type == 3 && ((p[0] >> 1) & 3)) {
    size_t topic = (p[pos] << 8) | p[pos + 1];
    pos += 2 + topic;
    if (!server.no_puback)
      append({0x40, 2, p[pos], p[pos + 1]});
  }
  if (type == 12)
    append({0xd0, 0});
  return n;
}
void NetworkClient::stop() {
  live = false;
  ++server.closes;
}
