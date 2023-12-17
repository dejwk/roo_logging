#include <Arduino.h>
#include <roo_logging.h>

void setup() {}

void loop() {
  LOG(INFO) << "Log at INFO level";
  LOG(WARNING) << "Log at WARNING level";
  LOG(ERROR) << "Log at ERROR level";

  SET_ROO_FLAG(roo_logging_minloglevel, 2);  // ERROR and above.

  LOG(INFO) << "Log at INFO level";
  LOG(WARNING) << "Log at WARNING level";
  LOG(ERROR) << "Log at ERROR level";

  delay(2000);

  SET_ROO_FLAG(roo_logging_minloglevel, 0);  // INFO and above.
}
