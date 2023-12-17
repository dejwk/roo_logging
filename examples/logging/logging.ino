#include <Arduino.h>

#include <roo_logging.h>

void setup() {}

void loop() {
  LOG(INFO) << "Log at INFO level";
  LOG(WARNING) << "Log at WARNING level";
  LOG(ERROR) << "Log at ERROR level";

  delay(2000);
  // This will cause the program to abort.
  LOG(FATAL) << "Log at FATAL level";
}
