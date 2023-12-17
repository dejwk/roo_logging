#include <Arduino.h>
#include <roo_logging.h>

void setup() {}

int num_cookies = 0;

void loop() {
  int size = random() % 2048;
  LOG(INFO) << "num_cookies: " << num_cookies;
  LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
  LOG_EVERY_N(INFO, 10) << "Got the " << roo_logging::COUNTER << "th cookie";
  LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << roo_logging::COUNTER
                                          << "th cookie, big one!";

  delay(500);
  ++num_cookies;
}
