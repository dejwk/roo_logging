#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

#include "gtest/gtest.h"
#include "roo_logging/stream.h"

namespace roo_logging {
namespace {

// Compares either floating-point overload with the original promoted %g path.
template <typename T>
void ExpectPrintfFloatingPoint(T value) {
  char expected[32];
  const int size = std::snprintf(expected, sizeof(expected), "%g",
                                 static_cast<double>(value));
  ASSERT_GT(size, 0);
  ASSERT_LT(static_cast<size_t>(size), sizeof(expected));
  char text[32];
  DefaultLogStream stream(text, sizeof(text));
  stream << value;
  EXPECT_EQ(std::string(expected), std::string(text, stream.pcount()));
}

// Verifies six significant digits, rounding across notation thresholds, and
// independence from the integer base for both float and double overloads.
TEST(LoggingFloatingPoint, FormatsGeneralNotation) {
  struct Case {
    double value;
    const char* expected;
  };
  const Case cases[] = {
      {1.23456789, "1.23457"}, {-1.23456789, "-1.23457"},
      {12.5, "12.5"},          {100.0, "100"},
      {0.0001, "0.0001"},      {0.00001, "1e-05"},
      {999999.0, "999999"},    {999999.5, "1e+06"},
      {1000000.0, "1e+06"},    {100000.5, "100000"},
      {100001.5, "100002"},    {-100000.5, "-100000"},
  };
  for (const Case& c : cases) {
    char text[64];
    DefaultLogStream stream(text, sizeof(text));
    stream << hex << c.value << ' ' << static_cast<float>(c.value) << ' ' << 31;
    EXPECT_EQ(std::string(c.expected) + " " + c.expected + " 1F",
              std::string(text, stream.pcount()));
  }
  // This float lies just above a six-digit rounding boundary when promoted.
  ExpectPrintfFloatingPoint(1.234565f);
}

// Verifies signed zero, infinities, signed NaNs, subnormals, and range limits
// retain the existing spelling for each source type.
TEST(LoggingFloatingPoint, HandlesSpecialValuesAndExtremes) {
  const double doubles[] = {
      0.0,
      -0.0,
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::quiet_NaN(),
      std::copysign(std::numeric_limits<double>::quiet_NaN(), -1.0),
      std::numeric_limits<double>::denorm_min(),
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::lowest(),
  };
  for (double value : doubles) ExpectPrintfFloatingPoint(value);
  const float floats[] = {
      0.0f,
      -0.0f,
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::quiet_NaN(),
      std::copysign(std::numeric_limits<float>::quiet_NaN(), -1.0f),
      std::numeric_limits<float>::denorm_min(),
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::lowest(),
  };
  for (float value : floats) ExpectPrintfFloatingPoint(value);
}

// Verifies rounding across a deterministic sample of binary32 and binary64
// encodings, including very small and very large magnitudes.
TEST(LoggingFloatingPoint, MatchesPrintfForBitPatterns) {
  static_assert(sizeof(float) == sizeof(uint32_t), "binary32 required");
  static_assert(sizeof(double) == sizeof(uint64_t), "binary64 required");
  uint64_t bits = 0x73013a543f38b46dULL;
  for (int i = 0; i < 4096; ++i) {
    bits ^= bits << 13;
    bits ^= bits >> 7;
    bits ^= bits << 17;
    double double_value;
    std::memcpy(&double_value, &bits, sizeof(double_value));
    ExpectPrintfFloatingPoint(double_value);
    const uint32_t float_bits = static_cast<uint32_t>(bits);
    float float_value;
    std::memcpy(&float_value, &float_bits, sizeof(float_value));
    ExpectPrintfFloatingPoint(float_value);
  }
}

// Verifies exact rounding at and immediately around decimal halfway values
// across the full exponent range, where approximate scaling can lose a digit.
TEST(LoggingFloatingPoint, RoundsAtDecimalBoundaries) {
  for (int exponent = -323; exponent <= 308; ++exponent) {
    const double scale = std::pow(10.0, exponent);
    for (double coefficient :
         {1.0, 1.234565, 9.999995, 1.000005, 1.999995, 2.000005}) {
      const double value = coefficient * scale;
      ExpectPrintfFloatingPoint(value);
      ExpectPrintfFloatingPoint(-value);
      ExpectPrintfFloatingPoint(std::nextafter(value, 0.0));
      ExpectPrintfFloatingPoint(
          std::nextafter(value, std::numeric_limits<double>::infinity()));
    }
  }
}

// Verifies truncation at every character preserves previous text, the stream
// cursor, and guard bytes for both floating-point overloads.
TEST(LoggingFloatingPoint, BoundedAppend) {
  const std::string expected = "x-1.23457e-20";
  for (size_t capacity = 1; capacity <= expected.size() + 2; ++capacity) {
    for (bool use_float : {false, true}) {
      char text[32];
      std::memset(text, 'z', sizeof(text));
      DefaultLogStream stream(text, capacity);
      stream << 'x';
      if (use_float) {
        stream << -1.23456789e-20f;
      } else {
        stream << -1.23456789e-20;
      }
      const size_t count =
          expected.size() < capacity - 1 ? expected.size() : capacity - 1;
      EXPECT_EQ(count, stream.pos_);
      EXPECT_EQ(expected.substr(0, count), std::string(text, stream.pcount()));
      EXPECT_EQ('z', text[capacity]);
      stream << "more" << 1.25f << 1.25;
      EXPECT_LE(stream.pos_, capacity - 1);
      EXPECT_EQ('z', text[capacity]);
    }
  }
}

#if !defined(ARDUINO)
// Verifies a portable stream with no storage accepts floating-point insertion.
TEST(LoggingFloatingPoint, ZeroCapacity) {
  DefaultLogStream stream(nullptr, 0);
  stream << 1.25f << 1.25;
  EXPECT_EQ(0, stream.pcount());
}
#endif

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
