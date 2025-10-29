#include <STM32RTCplus.h>
#include <WiFi.h>

const char* ssid = "MyWiFi";
const char* pass = "12345678";

STM32RTCplus rtc("MSK-3");  // Москва

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  rtc.begin();

  if (rtc.isFirstBoot()) {
    Serial.println("First boot — NTP sync...");
    rtc.syncNTP() ? Serial.println("OK") : Serial.println("Failed");
  } else {
    Serial.println("RTC restored");
  }
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    struct tm t;
    if (rtc.getTime(t)) {
      Serial.printf("%04d-%02d-%02d %02d:%02d:%02d %s\n",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec,
        t.tm_isdst ? "(DST)" : "");
    }
  }
}