#include "STM32RTCplus.h"

STM32RTCplus::STM32RTCplus(const char* tz) : _timezone(tz) {}

void STM32RTCplus::begin() {
  _rtc.begin();
  if (_magic1.read() != MAGIC1 || _magic2.read() != MAGIC2) {
    _magic1.write(MAGIC1);
    _magic2.write(MAGIC2);
  }
}

bool STM32RTCplus::isFirstBoot() {
  return _magic1.read() != MAGIC1 || _magic2.read() != MAGIC2;
}

bool STM32RTCplus::setTime(uint16_t y, uint8_t mo, uint8_t d,
                           uint8_t h, uint8_t mi, uint8_t s) {
  if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;
  _writeRefDate(y, mo, d);
  uint32_t daySec = h * 3600UL + mi * 60UL + s;
  _rtc.setSeconds(daySec);
  return true;
}

bool STM32RTCplus::getTime(struct tm &tm) {
  uint16_t ry, rmo, rd;
  if (!_readRefDate(ry, rmo, rd)) return false;

  uint32_t refDays = _dateToDays(ry, rmo, rd);
  uint32_t rtcSec = _rtc.getSeconds();
  uint32_t utcSec = refDays * 86400UL + rtcSec;

  uint32_t localSec = utcSec;
  uint16_t y = 1970; uint8_t mo = 1, d = 1, h = 0;
  int32_t offset = 0;

  for (int i = 0; i < 3; i++) {
    uint32_t days = localSec / 86400UL;
    uint32_t secs = localSec % 86400UL;
    _daysToDate(days, y, mo, d);
    h = secs / 3600;
    offset = _getTimezoneOffset(y, mo, d, h);
    localSec = utcSec + offset;
  }

  uint32_t days = localSec / 86400UL;
  uint32_t secs = localSec % 86400UL;
  _daysToDate(days, y, mo, d);

  tm.tm_year = y - 1900;
  tm.tm_mon  = mo - 1;
  tm.tm_mday = d;
  tm.tm_hour = secs / 3600;
  tm.tm_min  = (secs % 3600) / 60;
  tm.tm_sec  = secs % 60;
  tm.tm_wday = (days + 4) % 7;
  tm.tm_isdst = (offset != _getTimezoneOffset(y, mo, d, h));

  return true;
}

// === Универсальная NTP ===
bool STM32RTCplus::syncNTP(UDP &udp, bool (*isConnected)(), const char* server, int timeoutMs) {
  bool connected = true;
  if (isConnected != nullptr) {
    connected = isConnected();
  }
  if (!connected) return false;
  return _ntpSync(udp, true, server, timeoutMs);
}

bool STM32RTCplus::_ntpSync(UDP &udp, bool connected, const char* server, int timeoutMs) {
  const int NTP_PACKET_SIZE = 48;
  byte packet[NTP_PACKET_SIZE] = {0};

  packet[0] = 0b11100011;   // LI, Version, Mode
  packet[1] = 0;            // Stratum
  packet[2] = 6;            // Polling
  packet[3] = 0xEC;         // Precision
  packet[12] = 49;
  packet[13] = 0x4E;
  packet[14] = 49;
  packet[15] = 52;

  udp.beginPacket(server, 123);
  udp.write(packet, NTP_PACKET_SIZE);
  udp.endPacket();

  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    if (udp.parsePacket() >= NTP_PACKET_SIZE) {
      udp.read(packet, NTP_PACKET_SIZE);
      uint32_t high = word(packet[40], packet[41]);
      uint32_t low  = word(packet[42], packet[43]);
      uint32_t ntpTime = (high << 16) | low;
      uint32_t epoch = ntpTime - 2208988800UL;

      uint32_t days = epoch / 86400UL;
      uint32_t secs = epoch % 86400UL;
      uint16_t y; uint8_t mo, d;
      _daysToDate(days, y, mo, d);

      return setTime(y, mo, d,
                     secs / 3600,
                     (secs % 3600) / 60,
                     secs % 60);
    }
  }
  return false;
}

// === Коррекция времени ===
bool STM32RTCplus::adjustSeconds(int32_t seconds) {
  // Читаем текущее значение RTC-счётчика
  uint32_t current = _rtc.getSeconds();

  // Применяем поправку
  int64_t newSec = (int64_t)current + seconds;

  // Защита от переполнения
  if (newSec < 0) {
    newSec = 0;
  }
  if (newSec > 0xFFFFFFFFUL) {
    newSec = 0xFFFFFFFFUL;
  }

  // Записываем обратно в RTC — БЕЗ BKP!
  _rtc.setSeconds((uint32_t)newSec);

  return true;
}

// === Вспомогательные функции (без изменений) ===
uint32_t STM32RTCplus::_dateToDays(uint16_t y, uint8_t m, uint8_t d) {
  uint32_t days = 0;
  for (uint16_t yy = 1970; yy < y; yy++) days += _isLeapYear(yy) ? 366 : 365;
  const uint8_t dim[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  for (uint8_t mm = 1; mm < m; mm++) {
    days += dim[mm];
    if (mm == 2 && _isLeapYear(y)) days++;
  }
  return days + d - 1;
}

void STM32RTCplus::_daysToDate(uint32_t days, uint16_t &y, uint8_t &m, uint8_t &d) {
  y = 1970;
  while (days >= 365) {
    uint16_t yy = y;
    uint32_t diy = _isLeapYear(yy) ? 366 : 365;
    if (days < diy) break;
    days -= diy;
    y++;
  }
  m = 1;
  const uint8_t dim[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  while (days >= 28) {
    uint8_t dm = dim[m];
    if (m == 2 && _isLeapYear(y)) dm = 29;
    if (days < dm) break;
    days -= dm;
    m++;
  }
  d = days + 1;
}

bool STM32RTCplus::_isLeapYear(uint16_t y) {
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int32_t STM32RTCplus::_getTimezoneOffset(uint16_t y, uint8_t m, uint8_t d, uint8_t h) {
  // Поддержка UTC±N
  if (strncmp(_timezone, "UTC", 3) == 0) {
    int offset = 0;
    if (sscanf(_timezone + 3, "%d", &offset) == 1) {
      return offset * 3600;
    }
  }
  if (strcmp(_timezone, "MSK-3") == 0) return 3 * 3600;
  if (strcmp(_timezone, "CET-1") == 0) return 1 * 3600;
  if (strcmp(_timezone, "EST5EDT") == 0) {
    if (m > 3 && m < 11) return -4 * 3600;
    if (m < 3 || m > 11) return -5 * 3600;
    if (m == 3) {
      uint8_t sun = 8 + ((7 - (_dateToDays(y, 3, 1) + 4) % 7));
      if (d > sun || (d == sun && h >= 2)) return -4 * 3600;
    }
    if (m == 11) {
      uint8_t sun = 1 + ((7 - (_dateToDays(y, 11, 1) + 4) % 7));
      if (d >= sun) return -5 * 3600;
    }
    return -4 * 3600;
  }
  return 0;
}

void STM32RTCplus::_writeRefDate(uint16_t y, uint8_t m, uint8_t d) {
  uint32_t packed = ((uint32_t)y << 16) | (m << 8) | d;
  _refDate.write(packed);
}

bool STM32RTCplus::_readRefDate(uint16_t &y, uint8_t &m, uint8_t &d) {
  uint32_t p = _refDate.read();
  y = (p >> 16) & 0xFFFF;
  m = (p >> 8)  & 0xFF;
  d = p & 0xFF;
  return (y >= 1970 && m >= 1 && m <= 12 && d >= 1 && d <= 31);
}