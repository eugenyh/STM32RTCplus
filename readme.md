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

[For Wifi and Ethernet: STM32RTCplus\examples\BasicUsage](https://github.com/eugenyh/STM32RTCplus/tree/main/examples/BasicUsage)

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
| `bool adjustSeconds(int32_t seconds) ` | Returns true if time was adjusted (+ or - in sec) |

---

## 🌍 Supported Timezones

| Code | Description |
|------|--------------|
| `"UTC"` | Universal Coordinated Time (UTC+0), including UTC+N or UTC-N |
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
## Safe Usage of `syncNTP()` — Protect Your Backup Registers

The STM32F103's **BKP registers** have **~10,000 write cycles**.

## `syncNTP()` and `adjustSeconds()`

| Method | Writes to BKP? | Safe Frequency |
|-------|----------------|----------------|
| `syncNTP()` | Yes | **Once per day** |
| `adjustSeconds()` | No | **Unlimited** (every second if needed) |

### Best Practice

```cpp
// Full sync: once per day
if (rtc.isFirstBoot() || dailySync) {
  rtc.syncNTP(udp, isConnected);  // Writes BKP
}

// Drift correction: every hour
if (hourPassed) {
  rtc.adjustSeconds(+1);  // NO BKP wear
}
```

---


## ⚙️ Platform Support

- STM32F103C8 / STM32F103CB (“Blue Pill”, “Black Pill”)  
- Arduino IDE (STM32 core by ST or Roger Clark)  
- WiFi for NTP (ESP8266/ESP32 modules or Ethernet shield)

---

## 🧾 License

MIT License © 2025 EugenyH  
[GitHub: eugenyh/STM32RTCplus](https://github.com/eugenyh/STM32RTCplus)
