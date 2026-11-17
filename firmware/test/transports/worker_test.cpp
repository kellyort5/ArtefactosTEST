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
MqttTransport::Config config(const std::string &root,
                             const char *host = "sentinel.test") {
  MqttTransport::Config c;
  snprintf(c.host, sizeof(c.host), "%s", host);
  snprintf(c.ca, sizeof(c.ca), "%s", load(root).c_str());
  strcpy(c.user, "synthetic-user");
  strcpy(c.password, "synthetic-password");
  strcpy(c.client_id, "node-probe");
  strcpy(c.commands, "sentinel/v2/nodes/probe/commands");
  strcpy(c.availability, "sentinel/v2/nodes/probe/availability");
  c.time_valid = true;
  return c;
}
void prepare(const std::vector<std::string> &peer) {
  assert(transport.stopped());
  join_workers();
  reset_peer(peer);
  ups = downs = errors = messages = 0;
  got_retained = false;
  server.no_puback = false;
  server.omit_peer = false;
  server.inject_retained = false;
  server.gate = false;
}
void fail(const char *name, const std::vector<std::string> &peer,
          const MqttTransport::Config &c, bool omit = false) {
  prepare(peer);
  server.omit_peer = omit;
  assert(transport.start(c));
  wait_for([] { return transport.stopped() || ups.load() > 0; });
  if (!transport.stopped()) {
    transport.request_stop();
    wait_for([] { return transport.stopped(); });
  }
  join_workers();
  assert(server.sent.empty());
  assert(errors == 1);
  assert(server.closes >= 1);
  printf("%s: fail closed, ZERO MQTT/auth bytes PASS\n", name);
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
void success(const char *name, const std::vector<std::string> &peer,
             const MqttTransport::Config &c) {
  prepare(peer);
  server.inject_retained = true;
  assert(transport.start(c));
  wait_for([] { return ups == 1 && messages == 1; });
  assert(got_retained && server.actual_host == c.host);
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
  printf("%s: verified before auth, MQTT5/QoS1/retained input/stop PASS\n",
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
  auto rsa = config(r + "root.pem");
  fail("expired leaf", {f + "expired.der"}, rsa);
  fail("future leaf", {f + "future.der"}, rsa);
  fail("wrong DNS identity", {f + "valid.der"},
       config(r + "root.pem", "other.test"));
  fail("wrong CA", {f + "valid.der"}, config(r + "wrong-root.pem"));
  fail("missing retained peer chain", {f + "valid.der"}, rsa, true);
  auto ec = config(r + "chain-valid/root.pem");
  success("valid-DNS-chain-EC",
          {r + "chain-valid/leaf.der", r + "chain-valid/intermediate.der"}, ec);
  success("valid-IPv4-chain-EC",
          {r + "chain-valid/leaf.der", r + "chain-valid/intermediate.der"},
          config(r + "chain-valid/root.pem", "192.0.2.1"));
  success("valid-IPv6-chain-EC",
          {r + "chain-valid/leaf.der", r + "chain-valid/intermediate.der"},
          config(r + "chain-valid/root.pem", "2001:db8::1"));
  fail("expired intermediate",
       {r + "chain-expired/leaf.der", r + "chain-expired/intermediate.der"},
       config(r + "chain-expired/root.pem"));
  fail("future intermediate",
       {r + "chain-future/leaf.der", r + "chain-future/intermediate.der"},
       config(r + "chain-future/root.pem"));
  fail("expired configured CA", {r + "expired-ca/leaf.der"},
       config(r + "expired-ca/root.pem"));
  fail("future configured CA", {r + "future-ca/leaf.der"},
       config(r + "future-ca/root.pem"));
  success("valid-CA-keyCertSign", {r + "valid-ca-usage/leaf.der"},
          config(r + "valid-ca-usage/root.pem"));
  success("valid-bundle-unused-expired-CA",
          {r + "chain-valid/leaf.der", r + "chain-valid/intermediate.der"},
          config(r + "mixed-ca.pem"));
  prepare({f + "valid.der"});
  auto unknown = rsa;
  unknown.time_valid = false;
  assert(!transport.start(unknown));
  assert(!server.entered && server.sent.empty());
  puts("missing clock: refused before TLS/MQTT PASS");
  prepare({f + "valid.der"});
  server.gate = true;
  assert(transport.start(rsa));
  wait_for([] { return server.entered.load(); });
  transport.request_stop();
  assert(transport.stopping() && !transport.stopped());
  server.gate = false;
  wait_for([] { return transport.stopped(); });
  join_workers();
  assert(server.sent.empty());
  puts("cancel during I/O: supervisor calls return, pending until worker ACK, "
       "ZERO auth PASS");
  prepare({f + "valid.der"});
  server.no_puback = true;
  assert(transport.start(rsa));
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
  assert(transport.start(rsa));
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
