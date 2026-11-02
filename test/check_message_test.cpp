#include <memory>

#include "gtest/gtest.h"
#include "roo_logging/check.h"

// Verifies the caller owns a complete message after its builder and temporary
// stream have been destroyed, including both values and the closing delimiter.
TEST(CheckOpMessageBuilder, MessageOutlivesBuilder) {
  std::unique_ptr<roo_logging::StringType> message;
  {
    roo_logging::CheckOpMessageBuilder builder("actual == expected");
    *builder.ForVar1() << 42;
    *builder.ForVar2() << 43;
    message.reset(builder.NewString());
  }
  ASSERT_NE(message, nullptr);
  EXPECT_STREQ(message->c_str(), "actual == expected (42 vs. 43)");
}
