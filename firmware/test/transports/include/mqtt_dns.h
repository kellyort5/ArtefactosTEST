#pragma once
#include "IPAddress.h"
#include <atomic>
namespace sentinel {
class MqttDns {
public:
  bool resolve(const char *, IPAddress &out, const std::atomic<bool> &cancel) {
    out.value = "192.0.2.1";
    return !cancel;
  }
};
} // namespace sentinel
