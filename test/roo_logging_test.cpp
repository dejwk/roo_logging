#include "roo_logging.h"

#include "gtest/gtest.h"
#include "roo_logging/sink.h"

// Helper to capture log output.
#include <sstream>
#include <string>

struct StaticLogTest {
  StaticLogTest() { LOG(INFO) << "Foo"; }
} static_test;

class LogCapture : public roo_logging::LogSink {
 public:
  LogCapture() { roo_logging::SetSink(this); }
  ~LogCapture() { roo_logging::SetSink(nullptr); }
  std::string str() const { return ss_.str(); }
  void clear() { ss_.clear(); }

  void send(roo_logging::LogSeverity severity, const char* full_filename,
            const char* base_filename, int line, roo_time::Uptime uptime,
            roo_time::WallTime walltime, const char* message,
            size_t message_len) override {
    ss_ << full_filename << ":" << line << ": " << message;
  }

 private:
  std::stringstream ss_;
};

TEST(Logging, SimpleLogging) {
  LogCapture capture;
  LOG(INFO) << "Foo";
  std::string output = capture.str();
  EXPECT_NE(output.find("Foo"), std::string::npos);
}

TEST(Logging, WarningLogging) {
  LogCapture capture;
  LOG(WARNING) << "This is a warning message";
  std::string output = capture.str();
  EXPECT_NE(output.find("This is a warning message"), std::string::npos);
}

TEST(Logging, ErrorLogging) {
  LogCapture capture;
  LOG(ERROR) << "This is an error message";
  std::string output = capture.str();
  EXPECT_NE(output.find("This is an error message"), std::string::npos);
}

void boo() { CHECK(false) << "Boo!"; }

TEST(Logging, FatalLogging) {
  EXPECT_DEATH({ boo(); }, "Boo!");
}

TEST(Logging, MultipleMessages) {
  LogCapture capture;
  LOG(INFO) << "First message";
  LOG(WARNING) << "Second message";
  LOG(ERROR) << "Third message";
  std::string output = capture.str();
  EXPECT_NE(output.find("First message"), std::string::npos);
  EXPECT_NE(output.find("Second message"), std::string::npos);
  EXPECT_NE(output.find("Third message"), std::string::npos);
}

TEST(Logging, LoggingWithVariables) {
  LogCapture capture;
  int value = 42;
  LOG(INFO) << "Value is: " << value;
  std::string output = capture.str();
  EXPECT_NE(output.find("Value is: 42"), std::string::npos);
}

TEST(Logging, LoggingWithStringLiterals) {
  LogCapture capture;
  LOG(INFO) << "A simple string literal";
  std::string output = capture.str();
  EXPECT_NE(output.find("A simple string literal"), std::string::npos);
}

TEST(Logging, LoggingWithEmptyMessage) {
  LogCapture capture;
  LOG(INFO) << "";
  std::string output = capture.str();
  EXPECT_FALSE(output.empty());
}

// CHECK macros tests
TEST(Check, CheckTrueDoesNotFail) {
  EXPECT_NO_THROW({ CHECK(true); });
}

TEST(Check, CheckFalseThrowsOrAborts) {
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEATH({ CHECK(false); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckEqPasses) {
  int a = 5, b = 5;
  EXPECT_NO_THROW({ CHECK_EQ(a, b); });
}

TEST(Check, CheckEqFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 5, b = 6;
  EXPECT_DEATH({ CHECK_EQ(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckNePasses) {
  int a = 5, b = 6;
  EXPECT_NO_THROW({ CHECK_NE(a, b); });
}

TEST(Check, CheckNeFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 7, b = 7;
  EXPECT_DEATH({ CHECK_NE(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckLtPasses) {
  int a = 3, b = 4;
  EXPECT_NO_THROW({ CHECK_LT(a, b); });
}

TEST(Check, CheckLtFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 4, b = 3;
  EXPECT_DEATH({ CHECK_LT(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckLePasses) {
  int a = 4, b = 4;
  EXPECT_NO_THROW({ CHECK_LE(a, b); });
}

TEST(Check, CheckLeFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 5, b = 4;
  EXPECT_DEATH({ CHECK_LE(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckGtPasses) {
  int a = 5, b = 4;
  EXPECT_NO_THROW({ CHECK_GT(a, b); });
}

TEST(Check, CheckGtFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 3, b = 4;
  EXPECT_DEATH({ CHECK_GT(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}

TEST(Check, CheckGePasses) {
  int a = 4, b = 4;
  EXPECT_NO_THROW({ CHECK_GE(a, b); });
}

TEST(Check, CheckGeFails) {
#if GTEST_HAS_DEATH_TEST
  int a = 3, b = 4;
  EXPECT_DEATH({ CHECK_GE(a, b); }, "Check failed");
#else
  SUCCEED();
#endif
}
