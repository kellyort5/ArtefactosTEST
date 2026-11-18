#pragma once
#include "NetworkClient.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
struct FakeState {
  std::mutex lock;
  std::vector<std::vector<unsigned char>> sent;
  std::deque<unsigned char> incoming;
  std::vector<unsigned char> retained_packet;
  bool fail_connect = false, no_puback = false, inject_retained = false;
  std::atomic<bool> gate{false}, entered{false};
  unsigned closes = 0;
  std::string actual_ip;
};
extern FakeState server;
void join_workers();
void reset_peer();
