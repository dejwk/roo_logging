#include "roo_logging/buffered_log_stream.h"

#include <cstdarg>
#include <cstdio>

namespace roo_logging {
size_t BufferedLogStream::printf(const char* format, ...) {
  const size_t available = remaining_capacity();
  if (available == 0) return 0;
  va_list arguments;
  va_start(arguments, format);
  const int length =
      std::vsnprintf(buf_ + pos_, available + 1, format, arguments);
  va_end(arguments);
  if (length <= 0) return 0;
  const size_t written = static_cast<size_t>(length) < available
                             ? static_cast<size_t>(length)
                             : available;
  pos_ += written;
  return written;
}

size_t BufferedLogStream::printUnsigned(unsigned long long value,
                                        uint8_t base) {
  char buffer[8 * sizeof(value)];
  char* end = buffer + sizeof(buffer);
  char* current = end;
  do {
    const unsigned digit = value % base;
    *--current = digit < 10 ? '0' + digit : 'A' + digit - 10;
    value /= base;
  } while (value != 0);
  return write(current, end - current);
}
}  // namespace roo_logging
