// Defining NDEBUG disables debug logging. The debug logging macros (used below)
// are compiled away to nothing. LOG(DFATAL) turns into LOG(ERROR); i.e. the
// error gets logged but the program is not aborted.
//
// Normally, you should not define NDEBUG here, because it only applies to the
// file in which it is defined. Instead, prefer setting it in your build
// configuration. If using PlatformIO, add 'build_flags = -DNDEBUG' to
// platformio.ini.
//
// In simple sketches that use only a single file, putting the definition at the
// beginning of the sketch is a fair game.
#define NDEBUG

#include <Arduino.h>
#include <roo_logging.h>

void setup() {}

void loop() {
  // Debug macros (starting with 'D') get compiled away when NDEBUG is defined.
  DLOG(INFO) << "Found cookies";
  DCHECK_EQ(1, 2) << "Fails in debug mode";

  DLOG(FATAL) << "Gets compiled away in non-debug mode";

  // LOG(DFATAL) becomes LOG(ERROR) when NDEBUG is defined.
  LOG(DFATAL) << "Error, log it always, but crash in debug mode only";
  delay(1000);
}
