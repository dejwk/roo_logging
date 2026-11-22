#include "pico/stdlib.h"
#include "roo_logging.h"

int main() {
  stdio_init_all();
  for (;;) {
    LOG(INFO) << "Pico SDK logging";
    sleep_ms(1000);
  }
}
