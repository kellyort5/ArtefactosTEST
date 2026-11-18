#pragma once
#include <cstddef>
#include <cstdio>
#include <cstring>
namespace sentinel {
// Local test display only; never feed this writer into telemetry or storage.
class TestDisplay {
public:
  using Writer = bool (*)(const char *, size_t);
private:
  bool enabled;
  Writer writer;
  char previous[192] = {};
public:
  TestDisplay(bool on, Writer output) : enabled(on), writer(output) {}
  bool active() const { return enabled && writer; }
  bool status(const char *a, const char *b, const char *c, const char *d) {
    if (!active()) return false;
    char line[192];
    int n = std::snprintf(line, sizeof(line), "[P1 DISPLAY] %.32s | %.32s | %.48s | %.48s\n", a,b,c,d);
    if (n < 0 || size_t(n) >= sizeof(line)) return false;
    if (!std::strcmp(previous,line)) return true;
    if (!writer(line,size_t(n))) return false;
    std::memcpy(previous,line,size_t(n)+1);
    return true;
  }
  bool passkey(unsigned value, bool physical_window_open) {
    if (!active() || !physical_window_open || value > 999999) return false;
    char line[64] = {};
    int n = std::snprintf(line,sizeof(line),"[P1 PAIR CODE] %06u (temporary)\n",value);
    bool delivered = n > 0 && size_t(n) < sizeof(line) && writer(line,size_t(n));
    volatile char *clear = line;
    for (size_t i=0;i<sizeof(line);++i) clear[i]=0;
    return delivered;
  }
};
} // namespace sentinel
