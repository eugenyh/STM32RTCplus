# 🕒 STM32RTCplus

## Overview  
**STM32RTCplus** is an enhanced real-time clock (RTC) library for the **STM32F1** family (e.g. STM32F103) designed for **Arduino IDE (STM32Duino core)**.  
It extends the functionality of the built-in RTC peripheral.

---

## ✨ Features

- 🗓 Full date & time support (not only time counter)
- 🔋 Keeps both date and time while powered only by backup battery
- 🌐 One-time NTP synchronization
- 🕰 Y2038-safe timekeeping (no overflow in 2038)
- 🌍 Time zone support
- 💡 Arduino IDE compatible (no HAL or LL dependency)

---

## Installation  

### Option 1 — Clone into your Arduino libraries folder  
```bash
git clone https://github.com/eugenyh/STM32RTCplus.git ~/Documents/Arduino/libraries/STM32RTCplus
```

### Option 2 — Add as ZIP Library in Arduino IDE  
1. Download the latest release as a `.zip` file from GitHub.  
2. In Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library...**  
3. Select the downloaded file.  

Then include it:
```cpp
#include <STM32RTCplus.h>

STM32RTCplus rtc;
```

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

### 1. Hardware RTC Limitation
The STM32F1 RTC provides only a **32-bit seconds counter**, but **no calendar**.  
When powered by VBAT, it keeps counting — but **loses the date context**.

### 2. Reference Date + Counter = Full Time
`STM32RTCplus` stores:
- A **reference date** (year, month, day) in **RTC backup registers**
- The **RTC counter** holds seconds since that date

On startup:  
`Current time = reference date + RTC seconds`

→ Works even after months on battery.

### 3. Y2038-Safe (No 32-bit Unix Time)
- No `time_t`, no overflow in 2038
- Effective **64-bit precision**
- Overflow only after **~136 years**

### 4. NTP Synchronization
- `syncNTP()` fetches time from an NTP server
- Sets **new reference date** and **resets RTC counter**
- One-time operation (no intervals)

### 5. No Drift Correction
- No automatic crystal drift compensation
- Accuracy depends on 32.768 kHz crystal

---

## ⚙️ Platform Support

- STM32F103C8 / STM32F103CB (“Blue Pill”, “Black Pill”)  
- Arduino IDE (STM32 core by ST or Roger Clark)  
- WiFi for NTP (ESP8266/ESP32 modules or Ethernet shield)

---

## 🧾 License

MIT License © 2025 EugenyH  
[GitHub: eugenyh/STM32RTCplus](https://github.com/eugenyh/STM32RTCplus)
