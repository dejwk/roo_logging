#include <string>

#include "gtest/gtest.h"
#include "roo_logging/config.h"
#include "roo_logging/stacktrace.h"
#include "roo_logging/stderr.h"

namespace roo_logging {
namespace {

#if GTEST_HAS_STREAM_REDIRECTION
class LoggingStderrTest : public testing::Test {
 protected:
  void SetUp() override {
    saved_color_ = GET_ROO_FLAG(roo_logging_colorlogtostderr);
  }

  void TearDown() override {
    SET_ROO_FLAG(roo_logging_colorlogtostderr, saved_color_);
  }

 private:
  bool saved_color_;
};

// Verifies ANSI wrappers are correct for every severity and disabled coloring
// writes the same message bytes, including embedded NULs and bounded input.
TEST_F(LoggingStderrTest, WritesBoundedBytesWithOptionalColor) {
  struct Case {
    LogSeverity severity;
    const char* color_prefix;
  };
  const Case cases[] = {{ROO_LOGGING_INFO, ""},
                        {ROO_LOGGING_WARNING, "\033[0;33m"},
                        {ROO_LOGGING_ERROR, "\033[0;31m"},
                        {ROO_LOGGING_FATAL, "\033[0;31m"}};
  const char message[] = {'a', '\0', 'b', '!'};
  for (bool coloring : {false, true}) {
    SET_ROO_FLAG(roo_logging_colorlogtostderr, coloring);
    for (const Case& c : cases) {
      testing::internal::CaptureStderr();
      MaybeLogToStderr(c.severity, message, 3, false);
      const std::string output = testing::internal::GetCapturedStderr();
      const std::string prefix = coloring ? c.color_prefix : "";
      const std::string suffix = prefix.empty() ? "" : "\033[m";
      EXPECT_EQ(prefix + std::string(message, 3) + suffix, output);
    }
  }
}

// Verifies host static-initializer logging still targets stderr, including
// emulator builds where the ROM console would otherwise target stdout.
TEST_F(LoggingStderrTest, StaticInitializerUsesStderrOnHost) {
  SET_ROO_FLAG(roo_logging_colorlogtostderr, true);
  testing::internal::CaptureStderr();
  MaybeLogToStderr(ROO_LOGGING_WARNING, "early\n", 6, true);
  EXPECT_EQ("\033[0;33mearly\n\033[m", testing::internal::GetCapturedStderr());
}
#endif

#if defined(__linux__)
void CaptureStackLine(const char* text, void* output) {
  static_cast<std::string*>(output)->append(text);
}

// Verifies stack diagnostics retain complete lines and right-aligned pointer
// fields when addresses and symbols are assembled without snprintf.
TEST(LoggingStackTrace, FormatsAlignedAddresses) {
  std::string output;
  DumpStackTrace(0, CaptureStackLine, &output);
  ASSERT_FALSE(output.empty());
  size_t start = 0;
  while (start < output.size()) {
    const size_t end = output.find('\n', start);
    ASSERT_NE(std::string::npos, end);
    const std::string line = output.substr(start, end - start);
    ASSERT_EQ("    @ ", line.substr(0, 6));
    const size_t address = line.find("0x", 6);
    ASSERT_NE(std::string::npos, address);
    EXPECT_EQ(std::string(address - 6, ' '), line.substr(6, address - 6));
    size_t digit_end = address + 2;
    while (digit_end < line.size() &&
           ((line[digit_end] >= '0' && line[digit_end] <= '9') ||
            (line[digit_end] >= 'a' && line[digit_end] <= 'f'))) {
      ++digit_end;
    }
    EXPECT_GT(digit_end, address + 2);
    EXPECT_EQ(6 + 2 + 2 * sizeof(void*), digit_end);
    start = end + 1;
  }
}
#endif

}  // namespace
}  // namespace roo_logging
