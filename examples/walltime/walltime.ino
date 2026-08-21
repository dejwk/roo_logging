// ESP32 example, using its WiFi capabilities to read time via NTP.

// -- EMULATOR SETUP

#ifdef ROO_TESTING
#include <memory>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/wifi/wifi.h"

// Make the standard WiFi API connect to an in-process access point. Time is
// supplied by roo_testing's ESP32 time shim, which mirrors the host clock.
struct Emulator {
  roo_testing_transducers::wifi::Environment wifi;

  Emulator() {
    auto ap = std::make_unique<roo_testing_transducers::wifi::AccessPoint>(
        roo_testing_transducers::wifi::MacAddress(1, 1, 1, 1, 1, 1),
        "testwifi");
    ap->setAuthMode(roo_testing_transducers::wifi::AUTH_WEP);
    ap->setPasswd("pwd");
    wifi.addAccessPoint(std::move(ap));
    FakeEsp32().setWifiEnvironment(wifi);
  }
} emulator;

const char* wifi_ssid = "testwifi";
const char* wifi_password = "pwd";

#else

const char* wifi_ssid = "<your SSID>";
const char* wifi_password = "<your password>";

#endif

// -- EXAMPLE STARTS HERE

#include <Arduino.h>
#include <WiFi.h>
#include <roo_logging.h>
#include <roo_time.h>

using namespace roo_time;

const char* ntpServer = "pool.ntp.org";

// 2 hours behind UTC.
const TimeZone kLocalTz(Hours(2));

SystemClock my_clock;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  configTime(0, 0, ntpServer);

  DLOG(INFO) << "Still using uptime";

  SET_ROO_FLAG(roo_logging_wall_time_clock, &my_clock);
  SET_ROO_FLAG(roo_logging_timezone, kLocalTz);

  DLOG(INFO) << "Now using walltime";
}

void loop() {
  DLOG(INFO) << "Still using wall time";
  delay(1000);
}
