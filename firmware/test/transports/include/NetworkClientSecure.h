#pragma once
#include "IPAddress.h"
#include <cstddef>
#include <cstdint>
struct mbedtls_x509_buf {
  unsigned char *p;
  size_t len;
};
struct mbedtls_x509_crt {
  mbedtls_x509_buf raw;
  mbedtls_x509_crt *next;
};
class NetworkClientSecure {
  bool live = false;

public:
  void setTimeout(unsigned seconds);
  void setHandshakeTimeout(unsigned seconds);
  int connect(IPAddress, uint16_t, const char *, const char *, const char *,
              const char *);
  int available();
  int read(uint8_t *, size_t);
  size_t write(const uint8_t *, size_t);
  bool connected() { return live; }
  void stop();
  const mbedtls_x509_crt *getPeerCertificate();
};
