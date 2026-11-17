#pragma once
#include <NetworkClientSecure.h>
#include <ctime>
namespace sentinel {
// Native TLS must first validate CA and DNS/IP identity. No MQTT byte may be
// sent until this additional maintained X.509 engine accepts chain and dates.
bool verify_peer_dates(NetworkClientSecure &, const char *ca, time_t now,
                       bool time_valid);
} // namespace sentinel
