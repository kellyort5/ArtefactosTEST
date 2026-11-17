#pragma once
#include <cstddef>
#define MBEDTLS_PRIVATE(member) member
struct mbedtls_x509_crt { bool ca_istrue = false; };
void mbedtls_x509_crt_init(mbedtls_x509_crt *);
int mbedtls_x509_crt_parse(mbedtls_x509_crt *, const unsigned char *, size_t);
void mbedtls_x509_crt_free(mbedtls_x509_crt *);
// Host adapter uses the existing system OpenSSL parser; production uses mbedTLS.
