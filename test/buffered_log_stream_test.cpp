#include "roo_logging/buffered_log_stream.h"

#include <limits>
#include <string>

#include "gtest/gtest.h"

// Verifies all integer widths and the most negative signed value format safely.
TEST(BufferedLogStream, FormatsIntegersWithoutFramework) {
  char buffer[128];
  roo_logging::BufferedLogStream stream(buffer, sizeof(buffer));
  stream.print(-42, 10);
  stream.write(' ');
  stream.print(std::numeric_limits<long long>::min(), 10);
  stream.write(' ');
  stream.print(0xffffffffULL, 16);
  EXPECT_EQ(std::string(buffer, stream.pcount()),
            "-42 -9223372036854775808 FFFFFFFF");
}

// Verifies zero/tiny capacities and truncation leave borrowed storage bounded.
TEST(BufferedLogStream, BoundsOutput) {
  roo_logging::BufferedLogStream empty(nullptr, 0);
  EXPECT_FALSE(empty.write('x'));
  EXPECT_EQ(empty.printf("%s", "value"), 0u);
  char buffer[] = {'?', '?', '?', '?', '!'};
  roo_logging::BufferedLogStream stream(buffer, 4);
  EXPECT_EQ(stream.printf("%s", "abcdef"), 3u);
  EXPECT_EQ(std::string(buffer, stream.pcount()), "abc");
  EXPECT_EQ(buffer[3], '\0');
  EXPECT_EQ(buffer[4], '!');
  EXPECT_EQ(stream.print(123, 10), 0u);
}
