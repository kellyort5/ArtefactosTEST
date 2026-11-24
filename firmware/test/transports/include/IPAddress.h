#pragma once
#include <string>
struct IPAddress {
  std::string value;
  bool fromString(const char *s) {
    value = s;
    return true;
  }
};
