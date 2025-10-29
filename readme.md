# STM32RTCplus

## Overview  
**STM32RTCplus** is an enhanced real-time clock (RTC) library for the **STM32F1** family (e.g. STM32F103).  
It extends the functionality of the built-in RTC peripheral by adding:
- **Full calendar support** (date + time) even when running only from a backup battery.
- **Year 2038 fix** (avoids 32-bit Unix timestamp overflow).
- **Optional NTP synchronization** for accurate network time updates.
- Designed for **HAL** or **LL**-based projects with minimal overhead.

---

## Key Features  
✅ Full date & time tracking (Year, Month, Day, Hour, Minute, Second)  
✅ Persistent calendar in backup domain (works even after main power loss)  
✅ 64-bit timestamp logic – safe beyond the year 2038  
✅ Optional NTP synchronization over Ethernet or WiFi  
✅ Lightweight, dependency-free, and easy to integrate  
✅ Compatible with STM32 HAL and LL drivers  

---

## How It Works  

### 1. Hardware RTC integration  
The STM32F1 built-in RTC hardware provides only a **time counter** (seconds since last set) but **does not store calendar date** when powered by the backup battery alone.  
This library solves that limitation by maintaining a **software calendar layer** that translates the hardware tick counter into a full calendar date.

### 2. Persistent backup domain handling  
- When VBat is present, the hardware RTC continues to count seconds.  
- The library periodically saves **epoch offsets and date context** into RTC backup registers.  
- On startup, it reconstructs the full date/time from that stored context, even after long power losses.

### 3. 2038 problem (Unix timestamp overflow)  
Standard Unix time is a signed 32-bit integer (`time_t`), which overflows on **January 19, 2038**.  
`STM32RTCplus` internally uses **64-bit timestamps (uint64_t)**, ensuring correct operation for centuries ahead.

### 4. NTP Synchronization (optional)  
The library supports synchronization with an external **NTP server** over UDP.  
After network time is obtained, it adjusts the RTC and updates the backup domain accordingly.

### 5. Time drift correction  
Each synchronization recalculates offset and corrects for crystal drift automatically.  
You can define sync intervals (in minutes) or trigger manual NTP updates.

---

## Installation  

### Option 1 — Clone into your project  
```bash
git clone https://github.com/eugenyh/STM32RTCplus.git
```

### Option 2 — Add as a PlatformIO library  
Add this line into your `platformio.ini`:
```ini
lib_deps = https://github.com/eugenyh/STM32RTCplus.git
```

Then include it:
```cpp
#include "STM32RTCplus.h"
STM32RTCplus rtc;
```

---

## Usage Example  

```cpp
#include "STM32RTCplus.h"

STM32RTCplus rtc;

void setup() {
    rtc.begin();
    rtc.setDateTime(2025, 10, 29, 12, 0, 0);     // Set initial time
    rtc.enableNTP("pool.ntp.org", 60);           // Sync every 60 minutes
}

void loop() {
    DateTime now = rtc.getDateTime();

    Serial.print(now.year()); Serial.print("-");
    Serial.print(now.month()); Serial.print("-");
    Serial.print(now.day()); Serial.print(" ");
    Serial.print(now.hour()); Serial.print(":");
    Serial.print(now.minute()); Serial.print(":");
    Serial.println(now.second());

    delay(1000);
}
```

---

## Public API Reference  

| Method | Description |
|--------|--------------|
| `void begin()` | Initializes RTC hardware and loads backup data. |
| `void setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)` | Sets calendar date/time. |
| `DateTime getDateTime()` | Returns the current date/time. |
| `void enableNTP(const char* server, uint16_t intervalMin)` | Enables NTP sync with specified interval. |
| `void syncNow()` | Manually triggers immediate NTP synchronization. |
| `uint64_t getUnixTime64()` | Returns current time in 64-bit Unix timestamp. |
| `bool isValid()` | Checks RTC validity (whether initialized and running). |

---

## Compatibility  

| Item | Supported |
|------|------------|
| **MCU** | STM32F103 / STM32F1 series |
| **Frameworks** | STM32CubeIDE, HAL, LL, PlatformIO |
| **Networking** | Any module providing UDP (e.g. Ethernet, ESP8266, W5500, etc.) |
| **Memory footprint** | Very low (~2 KB Flash, <100 B RAM) |

---

## Internal Architecture  

```text
 ┌──────────────────────────────┐
 │        Application Code      │
 └──────────────┬───────────────┘
                │
                ▼
 ┌──────────────────────────────┐
 │       STM32RTCplus API       │
 │  - Calendar emulation        │
 │  - NTP synchronization       │
 │  - Drift compensation        │
 └──────────────┬───────────────┘
                │
                ▼
 ┌──────────────────────────────┐
 │     STM32 HAL / LL RTC       │
 │  - Hardware 32-bit counter   │
 │  - Backup domain registers   │
 └──────────────────────────────┘
```

---

## License  
Released under the **MIT License**.  
See [LICENSE](LICENSE) for details.

---

## Author  
Developed by **EugenyH** ([GitHub: eugenyh](https://github.com/eugenyh))  
Contributions, improvements, and pull requests are welcome!
