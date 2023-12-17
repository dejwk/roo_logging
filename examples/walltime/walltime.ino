// ESP32 example, using its WiFi capabilities to read time via NTP.

#include <Arduino.h>
#include <WiFi.h>
#include <roo_logging.h>
#include <roo_time.h>

using namespace roo_time;

const char* wifi_ssid = "<your SSID>";
const char* wifi_password = "<your password>";

const char* ntpServer = "pool.ntp.org";

// 2 hours behind UTC.
const TimeZone kLocalTz(Hours(2));

SystemClock my_clock;

void setup() {
  Serial.begin(9600);
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
