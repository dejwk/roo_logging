#include <Arduino.h>
#include <roo_logging.h>

// Must be no-return. (See 'debug logging' if you want checks not to crash in
// release mode).
void failure() {
  printf("In custom failure function\n");
  abort();
}

void setup() {
  roo_logging::InstallFailureFunction(&failure);
}

void loop() {
  CHECK_EQ(1, 2) << "Failing, as expected";
}
