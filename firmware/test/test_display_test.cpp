#include "test_display.h"
#include <cassert>
#include <string>
static std::string output;
static bool fail = false;
static unsigned calls = 0;
static bool write(const char *s, size_t n) { ++calls; if (fail) return false; output.append(s,n); return true; }
int main() {
  sentinel::TestDisplay disabled(false,write), missing(true,nullptr), serial(true,write);
  assert(!disabled.active() && !missing.active());
  assert(!disabled.passkey(123,true) && !disabled.status("idle","ready","DOWN",""));
  assert(output.empty());
  assert(!serial.passkey(123,false) && !serial.passkey(1000000,true));
  assert(output.empty());
  fail = true; assert(!serial.passkey(123,true));
  assert(!serial.status("idle","ready","DOWN",""));
  fail = false;
  assert(serial.status("idle","ready","DOWN",""));
  auto first = output; unsigned before = calls;
  assert(serial.status("idle","ready","DOWN","")); assert(calls == before);
  assert(serial.passkey(123,true)); assert(output.find("000123") != std::string::npos);
  output.clear();
  assert(serial.status("escudo","ready","DOWN",""));
  assert(output.find("000123") == std::string::npos);
  assert(output.size() < 192); // bounded public state; no passkey cached/replayed
  output.clear(); assert(!serial.passkey(123,false)); assert(output.empty());
}
