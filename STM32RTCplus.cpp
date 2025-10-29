#include "STM32RTCplus.h"
#include "stm32f1xx.h"  // Для RTC->CRL, RTC_CRL_CNF, RTC_CRL_RTOFF

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

  // 1. Сначала сохраняем опорную дату во Flash
  _writeRefDate(y, mo, d);

  // 2. Вычисляем секунды *текущего дня*
  uint32_t daySec = h * 3600UL + mi * 60UL + s;

  // 3. Устанавливаем счетчик RTC в это значение (с защитой от записи)
  _waitForLastTask();
  RTC->CRL |= RTC_CRL_CNF;  // Войти в режим конфигурации
  RTC->CNTH = daySec >> 16;
  RTC->CNTL = daySec & 0xFFFF;
  RTC->CRL &= ~RTC_CRL_CNF; // Выйти из режима
  _waitForLastTask();
  
  return true;
}

bool STM32RTCplus::getTime(struct tm &tm) {
  uint16_t ry, rmo, rd;
  if (!_readRefDate(ry, rmo, rd)) return false;

  uint32_t refDays = _dateToDays(ry, rmo, rd);
  uint32_t rtcSec = (RTC->CNTH << 16) | RTC->CNTL; // Читаем напрямую
  uint64_t utcSec = (uint64_t)refDays * 86400ULL + rtcSec;

  // ... (весь ваш код для часовых поясов, он без изменений) ...
  uint32_t localSec = utcSec;
  uint16_t y = 1970; uint8_t mo = 1, d = 1, h = 0;
  int32_t offset = 0;

  uint32_t finalDays = localSec / 86400UL;
  uint32_t finalSecs = localSec % 86400UL;
  _daysToDate(finalDays, y, mo, d);

  tm.tm_year = y - 1900;
  tm.tm_mon  = mo - 1;
  tm.tm_mday = d;
  tm.tm_hour = secs / 3600;
  tm.tm_min  = (secs % 3600) / 60;
  tm.tm_sec  = secs % 60;
  tm.tm_wday = (days + 4) % 7;
  // Небольшое исправление для tm_isdst
  int32_t currentOffset = _getTimezoneOffset(y, mo, d, tm.tm_hour);
  tm.tm_isdst = _isDST(y, mo, d, tm.tm_hour) ? 1 : 0;
  
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

bool STM32RTCplus::_isDST(uint16_t y, uint8_t m, uint8_t d, uint8_t h) {
  if (strcmp(_timezone, "EST5EDT") != 0) return false;
  if (m > 3 && m < 11) return true;
  if (m < 3 || m > 11) return false;
  if (m == 3) {
    uint8_t sun = 8 + ((7 - (_dateToDays(y, 3, 1) + 4) % 7));
    return (d > sun || (d == sun && h >= 2));
  }
  if (m == 11) {
    uint8_t sun = 1 + ((7 - (_dateToDays(y, 11, 1) + 4) % 7));
    return (d < sun);
  }
  return false;
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

  const int MAX_RETRIES = 3;
  const int RETRY_DELAY_MS = 500;

  for (int retry = 0; retry < MAX_RETRIES; retry++) {
    udp.beginPacket(server, 123);
    udp.write(packet, NTP_PACKET_SIZE);
    udp.endPacket();

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
      if (udp.parsePacket() >= NTP_PACKET_SIZE) {
        udp.read(packet, NTP_PACKET_SIZE);

        // === ВАЛИДАЦИЯ ПАКЕТА ===
        byte liVnMode = packet[0];
        byte vn = (liVnMode & 0b00111000) >> 3;
        byte mode = liVnMode & 0b00000111;
        if (vn != 3 && vn != 4) continue;  // NTP v3 или v4
        if (mode != 4) continue;          // Mode 4 = server
        if (packet[1] == 0) continue;     // Stratum 0 = ошибка

        // === ЧТЕНИЕ ВРЕМЕНИ ===
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

    // Если не получили ответ — ждём перед следующей попыткой
    if (retry < MAX_RETRIES - 1) {
      delay(RETRY_DELAY_MS);
    }
  }

  return false;  // Все попытки провалились
}


bool STM32RTCplus::adjustSeconds(int32_t seconds) {
  uint32_t current = (RTC->CNTH << 16) | RTC->CNTL;
  int64_t newSec = (int64_t)current + seconds;
  if (newSec < 0) newSec = 0;
  if (newSec > 0xFFFFFFFFUL) newSec = 0xFFFFFFFFUL;
  
  // Используем защиту от записи
  _waitForLastTask();
  RTC->CRL |= RTC_CRL_CNF;  // Войти в режим конфигурации
  RTC->CNTH = newSec >> 16;
  RTC->CNTL = newSec & 0xFFFF;
  RTC->CRL &= ~RTC_CRL_CNF; // Выйти из режима
  _waitForLastTask();
  
  return true;
}

void STM32RTCplus::_waitForLastTask() {
  // Ждем, пока RTC не будет готов к новой операции
  // (ждем установки флага RTOFF - Register Toggles OFF)
  while (!(RTC->CRL & RTC_CRL_RTOFF));
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
  // ... (код часовых поясов без изменений) ...
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