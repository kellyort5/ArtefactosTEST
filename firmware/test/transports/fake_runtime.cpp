#include "fake_runtime.h"
#include "Arduino.h"
#include "ESP_SSLClient.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <fstream>
#include <openssl/core_names.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
FakeState server;
// Isolated test clock: public certificates are deterministic fixtures.
extern "C" time_t time(time_t *out) noexcept {
  const time_t value = 1794631076;
  if (out)
    *out = value;
  return value;
}
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
void reset_peer(const std::vector<std::string> &paths) {
  server.ders.clear();
  server.chain.clear();
  for (auto &p : paths) {
    std::ifstream f(p, std::ios::binary);
    server.ders.emplace_back(std::istreambuf_iterator<char>(f),
                             std::istreambuf_iterator<char>());
  }
  server.chain.resize(paths.size());
  for (size_t i = 0; i < paths.size(); i++)
    server.chain[i] = {{server.ders[i].data(), server.ders[i].size()},
                       i + 1 < paths.size() ? &server.chain[i + 1] : nullptr};
  server.incoming.clear();
  server.sent.clear();
  server.closes = 0;
  server.entered = false;
  server.actual_host.clear();
}
void NetworkClientSecure::setTimeout(unsigned s) { assert(s == 1); }
void NetworkClientSecure::setHandshakeTimeout(unsigned s) { assert(s == 1); }
int NetworkClientSecure::connect(IPAddress, uint16_t port, const char *host,
                                 const char *ca, const char *cert,
                                 const char *key) {
  assert(port == 8883 && !cert && !key);
  server.entered = true;
  while (server.gate)
    delay(1);
  server.actual_host = host;
  X509_STORE *store = X509_STORE_new();
  BIO *b = BIO_new_mem_buf(ca, -1);
  X509 *root;
  while ((root = PEM_read_bio_X509(b, nullptr, nullptr, nullptr))) {
    X509_STORE_add_cert(store, root);
    X509_free(root);
  }
  BIO_free(b);
  STACK_OF(X509) *extra = sk_X509_new_null();
  X509 *leaf = nullptr;
  for (size_t i = 0; i < server.ders.size(); i++) {
    const unsigned char *p = server.ders[i].data();
    auto *x = d2i_X509(nullptr, &p, server.ders[i].size());
    if (!i)
      leaf = x;
    else
      sk_X509_push(extra, x);
  }
  X509_STORE_CTX *ctx = X509_STORE_CTX_new();
  int ok = leaf && X509_STORE_CTX_init(ctx, store, leaf, extra);
  if (ok) {
    auto *param = X509_STORE_CTX_get0_param(ctx);
    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_NO_CHECK_TIME);
    unsigned char ip[16];
    if (inet_pton(AF_INET, host, ip) == 1 || inet_pton(AF_INET6, host, ip) == 1)
      X509_VERIFY_PARAM_set1_ip_asc(param, host);
    else
      X509_VERIFY_PARAM_set1_host(param, host, 0);
    ok = X509_verify_cert(ctx) == 1;
  }
  X509_STORE_CTX_free(ctx);
  X509_free(leaf);
  sk_X509_pop_free(extra, X509_free);
  X509_STORE_free(store);
  live = ok;
  if (!ok)
    ++server.closes;
  return ok;
}
int NetworkClientSecure::available() {
  std::lock_guard<std::mutex> l(server.lock);
  return server.incoming.size();
}
int NetworkClientSecure::read(uint8_t *out, size_t n) {
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
size_t NetworkClientSecure::write(const uint8_t *p, size_t n) {
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
void NetworkClientSecure::stop() {
  live = false;
  ++server.closes;
}
const mbedtls_x509_crt *NetworkClientSecure::getPeerCertificate() {
  return server.omit_peer || server.chain.empty() ? nullptr : &server.chain[0];
}
namespace bssl {
X509List::X509List(const char *pem) {
  BIO *b = BIO_new_mem_buf(pem, -1);
  X509 *crt;
  while ((crt = PEM_read_bio_X509(b, nullptr, nullptr, nullptr))) {
    Item item;
    unsigned char *encoded = nullptr;
    int encoded_size = i2d_X509(crt, &encoded);
    der.emplace_back(encoded, encoded + encoded_size);
    OPENSSL_free(encoded);
    unsigned char *dn = nullptr;
    int dnlen = i2d_X509_NAME(X509_get_subject_name(crt), &dn);
    item.dn.assign(dn, dn + dnlen);
    OPENSSL_free(dn);
    EVP_PKEY *pub = X509_get_pubkey(crt);
    item.anchor.flags = BR_X509_TA_CA;
    if (EVP_PKEY_is_a(pub, "RSA")) {
      BIGNUM *n = nullptr, *e = nullptr;
      assert(EVP_PKEY_get_bn_param(pub, OSSL_PKEY_PARAM_RSA_N, &n) &&
             EVP_PKEY_get_bn_param(pub, OSSL_PKEY_PARAM_RSA_E, &e));
      item.n.resize(BN_num_bytes(n));
      BN_bn2bin(n, item.n.data());
      item.e.resize(BN_num_bytes(e));
      BN_bn2bin(e, item.e.data());
      BN_free(n);
      BN_free(e);
      item.anchor.pkey.key_type = BR_KEYTYPE_RSA;
    } else if (EVP_PKEY_is_a(pub, "EC")) {
      size_t n;
      assert(EVP_PKEY_get_octet_string_param(pub, OSSL_PKEY_PARAM_PUB_KEY,
                                             nullptr, 0, &n));
      item.q.resize(n);
      assert(EVP_PKEY_get_octet_string_param(pub, OSSL_PKEY_PARAM_PUB_KEY,
                                             item.q.data(), n, &n));
      item.anchor.pkey.key_type = BR_KEYTYPE_EC;
      item.anchor.pkey.key.ec.curve = BR_EC_secp256r1;
    }
    EVP_PKEY_free(pub);
    X509_free(crt);
    items.push_back(std::move(item));
  }
  BIO_free(b);
  for (auto &v : items) {
    auto a = v.anchor;
    a.dn = {v.dn.data(), v.dn.size()};
    if (a.pkey.key_type == BR_KEYTYPE_RSA)
      a.pkey.key.rsa = {v.n.data(), v.n.size(), v.e.data(), v.e.size()};
    else
      a.pkey.key.ec = {BR_EC_secp256r1, v.q.data(), v.q.size()};
    anchors.push_back(a);
  }
  for (auto &bytes : der)
    certificates.push_back({bytes.data(), bytes.size()});
}
} // namespace bssl
