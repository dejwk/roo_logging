#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

#include "gtest/gtest.h"
#include "roo_logging/stream.h"

namespace roo_logging {
namespace {

// Verifies pointer formatting preserves every address bit and ignores the
// selected integer base without changing it for subsequent values.
TEST(LoggingPointer, FormatsAddressesWithoutChangingBase) {
  char text[128];
  DefaultLogStream stream(text, sizeof(text));
  stream << oct << reinterpret_cast<const void*>(uintptr_t{0xabc}) << ' '
         << reinterpret_cast<const void*>(std::numeric_limits<uintptr_t>::max())
         << ' ' << 10;
  EXPECT_EQ("0xabc 0x" + std::string(2 * sizeof(uintptr_t), 'f') + " 12",
            std::string(text, stream.pcount()));
}

// Verifies both the pointer and nullptr overloads have the same null spelling.
TEST(LoggingPointer, FormatsNullAsHexZero) {
  char text[32];
  DefaultLogStream stream(text, sizeof(text));
  stream << static_cast<const void*>(nullptr) << ' ' << nullptr;
  EXPECT_EQ("0x0 0x0", std::string(text, stream.pcount()));
}

// Verifies truncation at every pointer character preserves existing text and
// buffer guards, including streams with no remaining payload capacity.
TEST(LoggingPointer, BoundedAppend) {
  const std::string expected = "x0xabcdef";
  for (size_t capacity = 1; capacity <= expected.size() + 2; ++capacity) {
    char text[32];
    std::memset(text, 'z', sizeof(text));
    DefaultLogStream stream(text, capacity);
    stream << 'x' << reinterpret_cast<const void*>(uintptr_t{0xabcdef});
    const size_t count =
        expected.size() < capacity - 1 ? expected.size() : capacity - 1;
    EXPECT_EQ(count, stream.pos_);
    EXPECT_EQ(expected.substr(0, count), std::string(text, stream.pcount()));
    EXPECT_EQ('z', text[capacity]);
    stream << "more";
    EXPECT_LE(stream.pos_, capacity - 1);
  }
}

// Verifies decimal padding, microseconds, and signed fractional-hour offsets
// retain the existing timestamp spelling across the supported year range.
TEST(LoggingDateTime, FormatsCalendarAndOffsets) {
  struct Case {
    roo_time::DateTime value;
    const char* expected;
  };
  const Case cases[] = {
      {roo_time::DateTime(1, 1, 2, 3, 4, 5, 6, roo_time::timezone::UTC),
       "0001-01-02T03:04:05.000006"},
      {roo_time::DateTime(2024, 2, 29, 12, 34, 56, 123456,
                          roo_time::timezone::UTC),
       "2024-02-29T12:34:56.123456"},
      {roo_time::DateTime(9999, 12, 31, 23, 59, 59, 999999,
                          roo_time::timezone::UTC),
       "9999-12-31T23:59:59.999999"},
      {roo_time::DateTime(2026, 11, 6, 0, 0, 0, 0,
                          roo_time::UtcOffset(roo_time::Minutes(345))),
       "2026-11-06T00:00:00.000000+05:45"},
      {roo_time::DateTime(2026, 11, 6, 0, 0, 0, 0,
                          roo_time::UtcOffset(roo_time::Minutes(-210))),
       "2026-11-06T00:00:00.000000-03:30"},
      {roo_time::DateTime(2026, 11, 6, 0, 0, 0, 0,
                          roo_time::UtcOffset(roo_time::Minutes(-30))),
       "2026-11-06T00:00:00.000000-00:30"},
  };
  for (const Case& c : cases) {
    char text[64];
    DefaultLogStream stream(text, sizeof(text));
    stream << hex << c.value << ' ' << 31;
    EXPECT_EQ(std::string(c.expected) + " 1F",
              std::string(text, stream.pcount()));
  }
}

// Verifies truncation at every timestamp/offset character keeps the stream
// cursor bounded and protects bytes beyond the borrowed capacity.
TEST(LoggingDateTime, BoundedAppend) {
  const roo_time::DateTime value(2026, 11, 6, 3, 4, 5, 6,
                                 roo_time::UtcOffset(roo_time::Minutes(-210)));
  const std::string expected = "x2026-11-06T03:04:05.000006-03:30";
  for (size_t capacity = 1; capacity <= expected.size() + 2; ++capacity) {
    char text[64];
    std::memset(text, 'z', sizeof(text));
    DefaultLogStream stream(text, capacity);
    stream << 'x' << value;
    const size_t count =
        expected.size() < capacity - 1 ? expected.size() : capacity - 1;
    EXPECT_EQ(count, stream.pos_);
    EXPECT_EQ(expected.substr(0, count), std::string(text, stream.pcount()));
    EXPECT_EQ('z', text[capacity]);
    stream << "more";
    EXPECT_LE(stream.pos_, capacity - 1);
  }
}

}  // namespace
}  // namespace roo_logging
