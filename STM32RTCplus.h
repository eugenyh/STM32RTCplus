#ifndef STM32RTCplus_h
#define STM32RTCplus_h

#include <Arduino.h>
#include <STM32RTC.h>
#include <FlashStorage.h>
#include <UDP.h>  // Универсальный UDP

class STM32RTCplus {
public:
  STM32RTCplus(const char* timezone = "UTC");

  void begin();
  bool setTime(uint16_t year, uint8_t month, uint8_t day,
               uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0);
  bool getTime(struct tm &tm);
  
  // Универсальная NTP синхронизация
  bool syncNTP(UDP &udp, 
               bool (*isConnected)() = nullptr,
               const char* server = "pool.ntp.org", 
               int timeoutMs = 5000);
  
  bool isFirstBoot();
  bool adjustSeconds(int32_t seconds);  // Добавлен метод коррекции

private:
  STM32RTC& _rtc = STM32RTC::getInstance();
  const char* _timezone;

  FlashStorage(_refDate, uint32_t);
  FlashStorage(_magic1, uint16_t);
  FlashStorage(_magic2, uint16_t);

  static const uint16_t MAGIC1 = 0xA5A5;
  static const uint16_t MAGIC2 = 0x5A5A;

  // NTP
  bool _ntpSync(UDP &udp, bool connected, const char* server, int timeoutMs);

  // Вспомогательные
  uint32_t _dateToDays(uint16_t y, uint8_t m, uint8_t d);
  void _daysToDate(uint32_t days, uint16_t &y, uint8_t &m, uint8_t &d);
  bool _isLeapYear(uint16_t y);
  int32_t _getTimezoneOffset(uint16_t y, uint8_t m, uint8_t d, uint8_t h);
  uint32_t _readRTC();
  void _writeRefDate(uint16_t y, uint8_t m, uint8_t d);
  bool _readRefDate(uint16_t &y, uint8_t &m, uint8_t &d);
};

#endif