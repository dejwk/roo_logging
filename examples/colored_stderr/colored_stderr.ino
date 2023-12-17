// For coloring to work, you need to configure your console to accept it.
// In PlatformIO, add 'monitor_raw = yes' to platformio.ini.

#include <Arduino.h>
#include <roo_logging.h>

void setup() {
  SET_ROO_FLAG(roo_logging_colorlogtostderr, true);
}

void loop() {
  LOG(INFO) << "INFO";        // Prints normal.
  LOG(WARNING) << "WARNING";  // Prints yellow.
  LOG(ERROR) << "ERROR";      // Prints red.
  delay(1000);
}
