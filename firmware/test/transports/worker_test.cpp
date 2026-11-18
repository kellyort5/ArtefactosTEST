#include "Arduino.h"
#include "fake_runtime.h"
#include "include/mqtt_transport.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
using sentinel::MqttTransport;
static std::string artifact_dir = "/tmp/sentinel-transport-tests/";
static MqttTransport transport;
std::atomic<int> ups{0}, downs{0}, errors{0}, messages{0};
std::atomic<bool> got_retained{false};
void state(bool up, bool error) {
  if (up)
    ++ups;
  else
    ++downs;
  if (error)
    ++errors;
}
void message(const char *p, size_t n, bool retain) {
  assert(std::string(p, n) == "{\"fixture\":true}");
  got_retained = retain;
  ++messages;
}
std::string load(const std::string &p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void wait_for(const std::function<bool()> &fn) {
  uint32_t end = millis() + 5000;
  while (!fn()) {
    assert((int32_t)(end - millis()) > 0);
    delay(1);
  }
}
MqttTransport::Config config(const char *host = "sentinel.test") {
  MqttTransport::Config c;
  snprintf(c.host, sizeof(c.host), "%s", host);
  strcpy(c.client_id, "node-probe");
  strcpy(c.commands, "sentinel/v2/nodes/probe/commands");
  strcpy(c.availability, "sentinel/v2/nodes/probe/availability");
  return c;
}
void prepare() {
  assert(transport.stopped());
  join_workers();
  reset_peer();
  ups = downs = errors = messages = 0;
  got_retained = false;
  server.no_puback = false;
  server.inject_retained = false;
  server.gate = false;
}
void save_frames(const char *name) {
  std::ofstream f(artifact_dir + name + ".hex");
  for (auto &frame : server.sent) {
    for (unsigned char b : frame) {
      char x[3];
      snprintf(x, sizeof(x), "%02x", b);
      f << x;
    }
    f << '\n';
  }
}
void success(const char *name, const MqttTransport::Config &c) {
  prepare();
  server.inject_retained = true;
  assert(transport.start(c));
  wait_for([] { return ups == 1 && messages == 1; });
  assert(got_retained && !server.actual_ip.empty());
  const char payload[] = "{\"stable_event\":\"fixture-1\"}";
  assert(transport.enqueue("sentinel/v2/nodes/probe/events", payload,
                           strlen(payload), 1, false));
  wait_for([] {
    std::lock_guard<std::mutex> l(server.lock);
    for (auto &p : server.sent)
      if ((p[0] >> 4) == 3)
        return true;
    return false;
  });
  transport.request_stop();
  wait_for([] { return transport.stopped(); });
  join_workers();
  assert(errors == 0);
  save_frames(name);
  printf("%s: TCP anonymous session, MQTT5/QoS1/retained input/stop PASS\n",
         name);
}
int main(int argc, char **argv) {
  assert(transport.init(state, message));
  std::string r = argc > 1 ? std::string(argv[1]) + "/"
                           : "/tmp/sentinel-transport-tests/fixtures/",
              f = r;
  if (argc > 2)
    artifact_dir = std::string(argv[2]) + "/";
  auto retained = load(r + "retained-command.bin");
  server.retained_packet.assign(retained.begin(), retained.end());
  auto tcp = config();
  success("valid-DNS", tcp);
  success("valid-IPv4", config("192.0.2.1"));
  success("valid-IPv6", config("2001:db8::1"));
  prepare();
  auto invalid = tcp; invalid.host[0] = 0;
  assert(!transport.start(invalid));
  invalid = tcp; invalid.port = 0;
  assert(!transport.start(invalid));
  assert(!server.entered && server.sent.empty());
  puts("missing host/port refused before TCP PASS");
  prepare(); server.fail_connect = true;
  assert(transport.start(tcp));
  wait_for([] { return transport.stopped(); }); join_workers();
  assert(errors == 1 && server.sent.empty());
  puts("TCP failure: no MQTT bytes sent PASS");
  prepare();
  server.gate = true;
  assert(transport.start(tcp));
  wait_for([] { return server.entered.load(); });
  transport.request_stop();
  assert(transport.stopping() && !transport.stopped());
  server.gate = false;
  wait_for([] { return transport.stopped(); });
  join_workers();
  assert(server.sent.empty());
  puts("cancel during I/O: supervisor calls return, pending until worker ACK, "
       "no CONNECT after cancel PASS");
  prepare();
  server.no_puback = true;
  assert(transport.start(tcp));
  wait_for([] { return ups == 1; });
  const char payload[] = "{\"stable_event\":\"replay-1\"}";
  assert(transport.enqueue("sentinel/v2/nodes/probe/events", payload,
                           strlen(payload), 1, false));
  wait_for([] { return transport.stopped(); });
  join_workers();
  save_frames("before-reconnect");
  assert(errors == 1);
  server.no_puback = false;
  server.incoming.clear();
  server.sent.clear();
  assert(transport.start(tcp));
  wait_for([] {
    std::lock_guard<std::mutex> l(server.lock);
    for (auto &p : server.sent)
      if ((p[0] >> 4) == 3)
        return true;
    return false;
  });
  delay(10);
  transport.request_stop();
  wait_for([] { return transport.stopped(); });
  join_workers();
  save_frames("after-reconnect");
  puts("unacknowledged QoS1: pending payload replayed after fresh "
       "authenticated session PASS");
}
