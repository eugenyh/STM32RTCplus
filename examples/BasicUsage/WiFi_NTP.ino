#include <STM32RTCplus.h>
#include <WiFi.h>

WiFiUDP udp;
STM32RTCplus rtc("MSK-3");

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("ssid", "pass");
  while (!isWiFiConnected()) delay(500);

  udp.begin(2390);
  rtc.begin();

  if (rtc.isFirstBoot()) {
    rtc.syncNTP(udp, isWiFiConnected);
  }
}