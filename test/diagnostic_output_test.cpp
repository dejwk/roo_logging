#include <cstdint>
#include <cstdlib>
#include <string>

#include "gtest/gtest.h"
#include "roo_logging/config.h"
#include "roo_logging/internal/stacktrace_format.h"
#include "roo_logging/stacktrace.h"
#include "roo_logging/stderr.h"

#if defined(ESP_PLATFORM) && !defined(ARDUINO) && GTEST_HAS_DEATH_TEST
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_logging/log_message.h"
#endif

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

#if defined(ESP_PLATFORM) && !defined(ARDUINO) && GTEST_HAS_DEATH_TEST
// Verifies a task queued before the scheduler starts is omitted from the
// startup prefix. The child exits with the queued task and its scheduler state.
TEST(LoggingStartup, IgnoresTaskQueuedBeforeSchedulerStarts) {
  EXPECT_EXIT(
      {
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
          std::_Exit(1);
        }
        TaskHandle_t pending_task = nullptr;
        const BaseType_t result =
            xTaskCreate([](void*) {}, "pending", configMINIMAL_STACK_SIZE,
                        nullptr, tskIDLE_PRIORITY, &pending_task);
        if (result != pdPASS || pending_task == nullptr ||
            xTaskGetCurrentTaskHandle() != pending_task) {
          std::_Exit(2);
        }

        SET_ROO_FLAG(roo_logging_prefix, true);
        SET_ROO_FLAG(roo_logging_wall_time_clock, nullptr);
        LogMessage("startup.cpp", 42).stream() << "before scheduler";
        std::_Exit(0);
      },
      testing::ExitedWithCode(0),
      R"(IS[+][0-9:.]+ startup[.]cpp:42\] before scheduler)");
}
#endif

#if defined(__linux__)
void CaptureStackLine(const char* text, void* output) {
  static_cast<std::string*>(output)->append(text);
}

// Verifies exact spacing, pointer text, symbol separation, and line endings
// with a known address, regardless of whether this build can unwind.
TEST(LoggingStackTrace, FormatsAlignedAddresses) {
  const void* pc = reinterpret_cast<const void*>(uintptr_t{0xabc});
  const std::string address =
      "    @ " + std::string(2 * sizeof(void*) - 3, ' ') + "0xabc";
  char text[100];
  internal::FormatStackTraceLine(text, sizeof(text), "    ", pc, nullptr);
  EXPECT_EQ(address + "\n", text);
  internal::FormatStackTraceLine(text, sizeof(text), "    ", pc, "Example()");
  EXPECT_EQ(address + "  Example()\n", text);
}

// Verifies that the real stack walker sends formatted frames to its writer.
TEST(LoggingStackTrace, DumpsFramesWhenUnwindingIsAvailable) {
  void* frame;
  if (internal::GetStackTrace(&frame, 1, 0) == 0) {
    GTEST_SKIP() << "This build cannot unwind the test stack";
  }

  std::string output;
  DumpStackTrace(0, CaptureStackLine, &output);
  ASSERT_FALSE(output.empty());
  EXPECT_EQ(0u, output.find("    @ "));
  EXPECT_EQ('\n', output.back());
}
#endif

}  // namespace
}  // namespace roo_logging
