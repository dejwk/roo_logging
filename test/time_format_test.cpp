#include <cstring>
#include <string>

#include "gtest/gtest.h"
#include "roo_logging.h"
#include "roo_time/format.h"

namespace roo_logging {
namespace {

// Captures the constructed prefix before the message body is appended.
std::string MakePrefix() {
  LogMessage message("time_format_test.cpp", 123);
  Stream& stream = message.stream();
  const std::string prefix(stream.buf_, stream.pcount());
  stream << "message";
  return prefix;
}

// Formats one streamed duration without log-prefix metadata.
std::string StreamDuration(roo_time::Duration value) {
  char text[64];
  DefaultLogStream stream(text, sizeof(text));
  stream << value;
  return std::string(text, stream.pcount());
}

// Verifies the timestamp has total hours and exactly six fractional digits.
void ExpectMicrosecondPrefix(const std::string& prefix) {
  ASSERT_FALSE(prefix.empty());
  EXPECT_EQ('I', prefix[0]);
  const size_t end = prefix.find(' ');
  ASSERT_NE(std::string::npos, end);
  const std::string marked = prefix.substr(1, end - 1);
  ASSERT_EQ(0u, marked.find("S+"));
  const std::string timestamp = marked.substr(2);
  ASSERT_GE(timestamp.size(), 15u);
  const size_t point = timestamp.size() - 7;
  EXPECT_EQ('.', timestamp[point]);
  EXPECT_EQ(':', timestamp[point - 3]);
  EXPECT_EQ(':', timestamp[point - 6]);
  for (size_t i = 0; i < timestamp.size(); ++i) {
    if (i == point || i == point - 3 || i == point - 6) continue;
    EXPECT_GE(timestamp[i], '0');
    EXPECT_LE(timestamp[i], '9');
  }
}

class LoggingTimeFormatTest : public testing::Test {
 protected:
  void SetUp() override {
    saved_format_ = GET_ROO_FLAG(roo_logging_uptime_format);
    saved_clock_ = GET_ROO_FLAG(roo_logging_wall_time_clock);
    saved_prefix_ = GET_ROO_FLAG(roo_logging_prefix);
    SET_ROO_FLAG(roo_logging_wall_time_clock, nullptr);
    SET_ROO_FLAG(roo_logging_prefix, true);
  }

  void TearDown() override {
    SET_ROO_FLAG(roo_logging_uptime_format, saved_format_);
    SET_ROO_FLAG(roo_logging_wall_time_clock, saved_clock_);
    SET_ROO_FLAG(roo_logging_prefix, saved_prefix_);
  }

 private:
  UptimeFormat saved_format_;
  WallTimeClockPtr saved_clock_;
  bool saved_prefix_;
};

// Verifies message durations use clock formatting and preserve fractional
// zeros.
TEST(LoggingDuration, ClockFormatAndFullRange) {
  using namespace roo_time;
  EXPECT_EQ("00:00:00", StreamDuration(Micros(0)));
  EXPECT_EQ("00:00:00.000001", StreamDuration(Micros(1)));
  EXPECT_EQ("00:00:00.001", StreamDuration(Millis(1)));
  EXPECT_EQ("00:00:01.5", StreamDuration(Millis(1500)));
  EXPECT_EQ("-00:00:00.001", StreamDuration(Millis(-1)));
  EXPECT_EQ("27:05:06", StreamDuration(Hours(27) + Minutes(5) + Seconds(6)));
  EXPECT_EQ("2562047788:00:54.775807", StreamDuration(Duration::Max()));
  EXPECT_EQ("-2562047788:00:54.775808", StreamDuration(Duration::Min()));
}

// Verifies streamed uptimes retain their marker and use total hours exactly.
TEST(LoggingUptime, DefaultClockAndFullRange) {
  using namespace roo_time;
  struct Case {
    Uptime value;
    const char* expected;
  };
  const Case cases[] = {
      {Uptime::Start(), "S+00:00:00.000000"},
      {Uptime::Start() + Hours(27) + Millis(1), "S+27:00:00.001000"},
      {Uptime::Start() - Micros(1), "S-00:00:00.000001"},
      {Uptime::Max(), "S+2562047788:00:54.775807"},
      {Uptime::Start() + Duration::Min(), "S-2562047788:00:54.775808"},
  };
  for (const Case& c : cases) {
    char text[64];
    DefaultLogStream stream(text, sizeof(text));
    stream << c.value;
    EXPECT_EQ(c.expected, std::string(text, stream.pcount()));
  }
}

// Verifies bounded uptime appends, including truncation within the domain
// marker.
TEST(LoggingUptime, BoundedAppendAndInvalidFormat) {
  const std::string expected = "xS-00:00:00.000001";
  for (size_t capacity = 1; capacity <= expected.size() + 2; ++capacity) {
    char text[32];
    std::memset(text, 'z', sizeof(text));
    DefaultLogStream stream(text, capacity);
    stream << 'x' << roo_time::Uptime::Start() - roo_time::Micros(1);
    const size_t count =
        expected.size() < capacity - 1 ? expected.size() : capacity - 1;
    EXPECT_EQ(count, stream.pos_);
    EXPECT_EQ(expected.substr(0, count), std::string(text, stream.pcount()));
    EXPECT_EQ('z', text[capacity]);
    EXPECT_EQ('\0', text[count]);
  }
  char text[32];
  DefaultLogStream stream(text, sizeof(text));
  stream << "head";
  EXPECT_FALSE(stream.formatUptime(roo_time::Uptime::Start(), "%Q"));
  EXPECT_FALSE(stream.formatUptime(roo_time::Uptime::Start(), nullptr));
  EXPECT_EQ(4u, stream.pos_);
  stream << "tail";
  EXPECT_EQ("headtail", std::string(text, stream.pcount()));
}

// Verifies component streaming uses the same default as duration streaming.
TEST(LoggingDuration, ComponentsUseClockFormat) {
  char text[64];
  DefaultLogStream stream(text, sizeof(text));
  stream << roo_time::Millis(1).toComponents();
  EXPECT_EQ("00:00:00.001", std::string(text, stream.pcount()));
}

// Verifies append/truncation behavior preserves cursor bounds and buffer
// guards.
TEST(LoggingDuration, BoundedAppend) {
  const std::string expected = "x-00:00:00.000001";
  for (size_t capacity = 1; capacity <= expected.size() + 2; ++capacity) {
    char text[32];
    std::memset(text, 'z', sizeof(text));
    DefaultLogStream stream(text, capacity);
    stream << 'x' << roo_time::Micros(-1);
    const size_t count =
        expected.size() < capacity - 1 ? expected.size() : capacity - 1;
    EXPECT_EQ(count, stream.pos_);
    EXPECT_EQ(expected.substr(0, count), std::string(text, stream.pcount()));
    EXPECT_EQ('\0', text[count]);
    EXPECT_EQ('z', text[capacity]);
    stream << "more";
    EXPECT_LE(stream.pos_, capacity - 1);
  }
}

// Verifies invalid format attempts preserve the existing logical stream text.
TEST(LoggingDuration, InvalidFormatDoesNotAdvance) {
  char text[32];
  DefaultLogStream stream(text, sizeof(text));
  stream << "head";
  EXPECT_FALSE(stream.formatDuration(roo_time::Seconds(1), "%Q"));
  EXPECT_EQ(4u, stream.pos_);
  EXPECT_FALSE(stream.formatDuration(roo_time::Seconds(1), nullptr));
  EXPECT_EQ(4u, stream.pos_);
  stream << "tail";
  EXPECT_EQ("headtail", std::string(text, stream.pcount()));
}

// Verifies printf remains bounded after a configured duration fills a stream.
TEST(LoggingDuration, PrintfAfterTruncation) {
  char text[10];
  std::memset(text, 'z', sizeof(text));
  DefaultLogStream stream(text, 9);
  EXPECT_EQ(8u, stream.printf("%s", "1234567890"));
  EXPECT_EQ(8u, stream.pos_);
  EXPECT_EQ(0u, stream.printf("%s", "more"));
  EXPECT_EQ(8u, stream.pos_);
  EXPECT_EQ('\0', text[8]);
  EXPECT_EQ('z', text[9]);
}

// Verifies the flag default and generated uptime prefix both use microseconds.
TEST_F(LoggingTimeFormatTest, DefaultMicrosecondPrefix) {
  EXPECT_STREQ(roo_time::kUptimeClockMicrosFormat,
               GET_ROO_FLAG(roo_logging_uptime_format));
  ExpectMicrosecondPrefix(MakePrefix());
}

// Verifies the flag is reread for each log and accepts duration padding
// directives.
TEST_F(LoggingTimeFormatTest, ConfigurablePrefix) {
  SET_ROO_FLAG(roo_logging_uptime_format, "clock=%T%F");
  const std::string first = MakePrefix();
  EXPECT_EQ(0u, first.find("Iclock="));
  EXPECT_EQ(std::string::npos, first.find('%'));
  SET_ROO_FLAG(roo_logging_uptime_format, "days=%03D");
  const std::string second = MakePrefix();
  EXPECT_EQ(0u, second.find("Idays="));
  EXPECT_EQ(std::string::npos, second.find('%'));
  EXPECT_GE(second.find(' '), 9u);
  SET_ROO_FLAG(roo_logging_uptime_format, "S%+%T%F");
  EXPECT_EQ(0u, MakePrefix().find("IS+"));
}

// Verifies changing log-prefix formatting does not change message durations.
TEST_F(LoggingTimeFormatTest, PrefixFlagDoesNotChangeDurations) {
  SET_ROO_FLAG(roo_logging_uptime_format, "custom");
  EXPECT_EQ(0u, MakePrefix().find("Icustom "));
  EXPECT_EQ("00:00:01", StreamDuration(roo_time::Seconds(1)));
  char text[64];
  DefaultLogStream stream(text, sizeof(text));
  stream << roo_time::Uptime::Start() + roo_time::Seconds(1);
  EXPECT_EQ("S+00:00:01.000000", std::string(text, stream.pcount()));
}

// Verifies malformed and null formats fall back without corrupting the prefix.
TEST_F(LoggingTimeFormatTest, InvalidPrefixFallsBack) {
  SET_ROO_FLAG(roo_logging_uptime_format, "broken-%Q");
  ExpectMicrosecondPrefix(MakePrefix());
  SET_ROO_FLAG(roo_logging_uptime_format, nullptr);
  ExpectMicrosecondPrefix(MakePrefix());
}

// Verifies the prefix flag is irrelevant when log prefixes are disabled.
TEST_F(LoggingTimeFormatTest, DisabledPrefix) {
  SET_ROO_FLAG(roo_logging_prefix, false);
  SET_ROO_FLAG(roo_logging_uptime_format, "custom");
  EXPECT_EQ("", MakePrefix());
}

class FixedTestClock : public roo_time::WallTimeClock {
 public:
  roo_time::WallTime now() const override {
    return roo_time::DateTime(2026, 10, 31, roo_time::timezone::UTC).wallTime();
  }
};

// Verifies wall-clock timestamps retain calendar formatting regardless of the
// flag.
TEST_F(LoggingTimeFormatTest, WallClockIgnoresUptimeFormat) {
  FixedTestClock clock;
  const roo_time::UtcOffset saved_offset = GET_ROO_FLAG(roo_logging_timezone);
  SET_ROO_FLAG(roo_logging_timezone, roo_time::timezone::UTC);
  SET_ROO_FLAG(roo_logging_wall_time_clock, &clock);
  SET_ROO_FLAG(roo_logging_uptime_format, "custom");
  EXPECT_EQ(0u, MakePrefix().find("I2026-10-31T00:00:00.000000 "));
  SET_ROO_FLAG(roo_logging_timezone, saved_offset);
}

// Verifies oversized formats safely exhaust the prefix buffer before other
// fields.
TEST_F(LoggingTimeFormatTest, OversizedPrefixIsBounded) {
  const std::string format(2 * kMaxLogMessageLen, 'x');
  SET_ROO_FLAG(roo_logging_uptime_format, format.c_str());
  const std::string prefix = MakePrefix();
  EXPECT_EQ(kMaxLogMessageLen - 1, prefix.size());
  EXPECT_EQ('I', prefix[0]);
  EXPECT_EQ(std::string(kMaxLogMessageLen - 2, 'x'), prefix.substr(1));
}

}  // namespace
}  // namespace roo_logging
