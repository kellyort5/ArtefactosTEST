#pragma once
#include <cstdint>
#include <cstdlib>
#include <sys/time.h>

namespace sentinel {
// Recibe UTC ya validado por el contrato. La conversión no depende de la zona
// horaria local; conserva hasta seis decimales sin redondear las fracciones.
inline timeval command_timeval(const char *s) {
  int year = atoi(s), month = atoi(s + 5), day = atoi(s + 8);
  auto leaps = [](int y) {
    --y;
    return y / 4 - y / 100 + y / 400;
  };
  int64_t days = 365LL * (year - 1970) + leaps(year) - leaps(1970) + day - 1;
  int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
    lengths[1] = 29;
  for (int m = 1; m < month; ++m)
    days += lengths[m - 1];
  timeval tv = {};
  tv.tv_sec =
      days * 86400 + atoi(s + 11) * 3600 + atoi(s + 14) * 60 + atoi(s + 17);
  if (s[19] == '.') {
    unsigned digits = 0;
    for (size_t i = 20; s[i] != 'Z' && digits < 6; ++i, ++digits)
      tv.tv_usec = tv.tv_usec * 10 + s[i] - '0';
    while (digits++ < 6)
      tv.tv_usec *= 10;
  }
  return tv;
}
} // namespace sentinel
