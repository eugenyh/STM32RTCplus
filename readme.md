# 🕒 STM32RTCplus

**Advanced RTC library for STM32F103 (Arduino core)**  
Adds full date/calendar support, safe time storage across power loss, and one-time NTP synchronization.

---

## ✨ Features

- 🗓 Full date & time support (not only time counter)
- 🔋 Keeps both date and time while powered only by backup battery
- 🌐 One-time NTP synchronization
- 🕰 Y2038-safe timekeeping (no overflow in 2038)
- 🌍 Time zone support
- 💡 Arduino IDE compatible (no HAL or LL dependency)

---

## 🚀 Usage Example

```cpp
#include <STM32RTCplus.h>
#include <WiFi.h>

const char* ssid = "MyWiFi";
const char* pass = "12345678";

STM32RTCplus rtc("MSK-3");  // Moscow timezone (UTC+3)

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  rtc.begin();

  if (rtc.isFirstBoot()) {
    Serial.println("First boot — syncing with NTP...");
    if (rtc.syncNTP()) {
      Serial.println("NTP sync successful");
    } else {
      Serial.println("NTP sync failed");
    }
  } else {
    Serial.println("RTC restored from backup");
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
```

---

## 🧩 Public API Reference

| Method | Description |
|--------|--------------|
| `STM32RTCplus(const char* timezone = "UTC")` | Constructor with timezone (e.g., `"MSK-3"`, `"CET-1"`) |
| `void begin()` | Initializes RTC and loads state from backup registers |
| `bool setTime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0)` | Sets current time and date (UTC) |
| `bool getTime(struct tm &tm)` | Gets current local time |
| `bool syncNTP(const char* server = "pool.ntp.org", uint16_t timeout = 5000)` | Performs one-time NTP synchronization |
| `bool isFirstBoot()` | Returns true if RTC backup area was empty (first initialization) |

---

## 🌍 Supported Timezones

| Code | Description |
|------|--------------|
| `"UTC"` | Universal Coordinated Time (UTC+0) |
| `"MSK-3"` | Moscow (UTC+3) |
| `"CET-1"` | Central Europe (UTC+1) |
| `"EST5EDT"` | Eastern US (auto DST) |

---

## 🧠 How It Works

STM32F103 has a hardware RTC, but it only keeps **seconds** — not the full date — while running from backup battery.  
`STM32RTCplus` solves this by:

- Storing **base date** (year, month, day) in **backup registers**
- Counting **seconds since base date** in the RTC counter  
→ Together, this acts like a **64-bit extended timestamp**, safe far beyond 2038.

---

## 🛡 Y2038-Safe Design

Instead of using 32-bit `time_t`, the library combines:
- **Reference date** stored in backup registers  
- **RTC seconds counter**  

This effectively provides **64-bit precision**, with overflow only after ~136 years of continuous operation.

---

## ⚙️ Platform Support

- STM32F103C8 / STM32F103CB (“Blue Pill”, “Black Pill”)  
- Arduino IDE (STM32 core by ST or Roger Clark)  
- WiFi for NTP (ESP8266/ESP32 modules or Ethernet shield)

---

## 🧾 License

MIT License © 2025 Eugeny Ho  
[GitHub: eugenyh/STM32RTCplus](https://github.com/eugenyh/STM32RTCplus)
