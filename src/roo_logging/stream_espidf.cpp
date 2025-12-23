#if defined(ESP_PLATFORM) && !defined(ARDUINO)

#include <stdarg.h>

#include "roo_logging/stream_espidf.h"

namespace roo_logging {

size_t EspidfLogStream::print(unsigned long n, uint8_t base) {
  char buf[8 * sizeof(n) + 1];  // Max for base = 2.
  char* str = &buf[sizeof(buf) - 1];

  *str = '\0';

  if (base < 2) base = 10;

  do {
    unsigned long m = n;
    n /= base;
    char c = m - n * base;
    if (c < 10) {
      c += '0';
    } else {
      c += 'A' - 10;
    }
    *--str = c;
  } while (n > 0);

  return write(str, strlen(str));
}

size_t EspidfLogStream::printf(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  int len = vsnprintf(buf_ + pos_, remaining_capacity(), format, arg);
  va_end(arg);
  pos_ += len;
  return len;
}

}  // namespace roo_logging

#endif  // defined(ESP_PLATFORM) && !defined(ARDUINO)