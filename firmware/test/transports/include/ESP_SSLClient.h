#pragma once
#define ESP_SSLCLIENT_VERSION_MAJOR 3
#define ESP_SSLCLIENT_VERSION_MINOR 1
#define ESP_SSLCLIENT_VERSION_PATCH 3
#include "bearssl.h"
#include <vector>
namespace bssl {
class X509List {
  struct Item {
    std::vector<unsigned char> dn, n, e, q;
    br_x509_trust_anchor anchor = {};
  };
  std::vector<Item> items;
  std::vector<br_x509_trust_anchor> anchors;
  std::vector<std::vector<unsigned char>> der;
  std::vector<br_x509_certificate> certificates;

public:
  explicit X509List(const char *);
  size_t getCount() const { return anchors.size(); }
  const br_x509_trust_anchor *getTrustAnchors() const { return anchors.data(); }
  const br_x509_certificate *getX509Certs() const {
    return certificates.data();
  }
};
} // namespace bssl
