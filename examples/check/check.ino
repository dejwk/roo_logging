#include <Arduino.h>

#include <roo_logging.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Equals.
  CHECK_EQ(2, 2) << "I sure hope so";

  // Not equals.
  CHECK_NE(1, 2) << "The world must be ending!";

  // Greater than.
  CHECK_GT(3, 2);

  // Greater than or equal to.
  CHECK_GE(3, 2);

  // Less than.
  CHECK_LT(1, 2);

  // Less than or equal to.
  CHECK_LE(1, 2);

  // Null-checks can be used inside expressions.
  auto* ptr = CHECK_NOTNULL(&Serial);
  ptr->println("OK");

  CHECK_EQ(String("abc")[1], 'b');

  // Check a predicate.
  CHECK(2 == 2);  // But prefer CHECK_EQ etc. when applicable.

  // This one fails.
  CHECK_GT(String("abba"), String("foo")) << "Oh well.";
}
