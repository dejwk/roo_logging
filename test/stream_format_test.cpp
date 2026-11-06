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

}  // namespace
}  // namespace roo_logging
