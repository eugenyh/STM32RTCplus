#include <STM32RTCplus.h>
#include <Ethernet.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetUDP udp;
STM32RTCplus rtc("UTC+5");

bool isEthernetConnected() {
  return Ethernet.linkStatus() == LinkON;
}

void setup() {
  Serial.begin(115200);
  Ethernet.begin(mac);
  udp.begin(8888);
  rtc.begin();

  if (rtc.isFirstBoot()) {
    rtc.syncNTP(udp, isEthernetConnected);
  }
}