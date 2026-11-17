#pragma once
#include "NetworkClientSecure.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
struct FakeState {
  std::mutex lock;
  std::vector<std::vector<unsigned char>> ders, sent;
  std::vector<mbedtls_x509_crt> chain;
  std::deque<unsigned char> incoming;
  std::vector<unsigned char> retained_packet;
  bool omit_peer = false, no_puback = false, inject_retained = false;
  std::atomic<bool> gate{false}, entered{false};
  unsigned closes = 0;
  std::string actual_host;
};
extern FakeState server;
void join_workers();
void reset_peer(const std::vector<std::string> &);
