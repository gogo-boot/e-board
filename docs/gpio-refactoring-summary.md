# GPIO Pin Refactoring - Completion Summary

**Date:** November 15, 2025
**Status:** ✅ COMPLETED

---

## Overview

Successfully refactored all GPIO pin-related variables to use a **dual-layer naming strategy** that combines both
physical GPIO IDs and logical functional names.

---

## Changes Made

### 1. **Refactored `include/config/pins.h`**

#### **Layer 1: Physical GPIO Assignments**

Physical GPIO pins are now defined with `GPIO_` prefix and `gpio_num_t` type:

```cpp
// ESP32-S3 Example:
constexpr gpio_num_t GPIO_BUTTON_1 = GPIO_NUM_2;
constexpr gpio_num_t GPIO_BATTERY_ADC = GPIO_NUM_1;
constexpr gpio_num_t GPIO_EPD_BUSY = GPIO_NUM_4;
```

#### **Layer 2: Functional Aliases**

Application code uses readable functional names that map to Layer 1:

```cpp
// ESP32-S3 Example:
constexpr int BUTTON_HALF_AND_HALF = GPIO_BUTTON_1;
constexpr int BATTERY_ADC = GPIO_BATTERY_ADC;
constexpr int EPD_BUSY = GPIO_EPD_BUSY;
```

#### **Removed:**

- Unused Arduino-style pin aliases (A0-A5, SCK, MISO, MOSI, SS, SDA, SCL, GPIO10, RX, TX)
- These were not used anywhere in the codebase and created confusion

#### **Added:**

- ESP32-C3 button GPIO placeholders for future use:
    - `GPIO_BUTTON_1 = GPIO_NUM_0`
    - `GPIO_BUTTON_2 = GPIO_NUM_1`
    - `GPIO_BUTTON_3 = GPIO_NUM_5`
- Comprehensive documentation comments
- `BUTTON_FACTORY_RESET` alias pointing to `GPIO_BUTTON_1` (same as `BUTTON_HALF_AND_HALF`)

---

### 2. **Updated `src/util/battery_manager.cpp`**

**Before:**

```cpp
static const int BATTERY_ADC_PIN = 1;
static const int ADC_EN_PIN = 6;
```

**After:**

```cpp
// Now uses centralized pin definitions
pinMode(Pins::BATTERY_ADC, INPUT);
pinMode(Pins::ADC_EN, OUTPUT);
digitalWrite(Pins::ADC_EN, HIGH);
analogRead(Pins::BATTERY_ADC);
```

**Benefit:** No more hardcoded GPIO numbers scattered in implementation files.

---

### 3. **Updated `include/util/factory_reset.h` & `src/util/factory_reset.cpp`**

**Before:**

```cpp
// In factory_reset.h
const gpio_num_t RESET_BUTTON_GPIO = GPIO_NUM_2;

// In factory_reset.cpp
digitalRead(RESET_BUTTON_GPIO)
```

**After:**

```cpp
// In factory_reset.h
#include "config/pins.h"

// In factory_reset.cpp
#ifdef BOARD_ESP32_S3
    digitalRead(Pins::BUTTON_FACTORY_RESET)
#endif
```

**Benefit:**

- Centralized pin definition
- Board-specific compilation
- Clear that factory reset shares GPIO 2 with the half-and-half button

---

## GPIO Pin Mapping Reference

### **ESP32-S3 Pin Assignments**

| GPIO | Physical Name         | Functional Name(s)                               | Purpose                                                                      |
|------|-----------------------|--------------------------------------------------|------------------------------------------------------------------------------|
| 1    | `GPIO_BATTERY_ADC`    | `BATTERY_ADC`                                    | Battery voltage sensor (2:1 divider)                                         |
| 2    | `GPIO_BUTTON_1`       | `BUTTON_HALF_AND_HALF`<br>`BUTTON_FACTORY_RESET` | Multi-function: Short press = mode switch<br>Long press (5s) = factory reset |
| 3    | `GPIO_BUTTON_2`       | `BUTTON_WEATHER_ONLY`                            | Weather-only display mode                                                    |
| 4    | `GPIO_EPD_BUSY`       | `EPD_BUSY`                                       | E-Paper busy signal                                                          |
| 5    | `GPIO_BUTTON_3`       | `BUTTON_DEPARTURE_ONLY`                          | Departure-only display mode                                                  |
| 6    | `GPIO_BATTERY_ADC_EN` | `ADC_EN`                                         | Battery ADC power control                                                    |
| 7    | `GPIO_EPD_SCK`        | `EPD_SCK`                                        | E-Paper SPI clock                                                            |
| 9    | `GPIO_EPD_SDI`        | `EPD_SDI`                                        | E-Paper SPI data (MOSI)                                                      |
| 10   | `GPIO_EPD_DC`         | `EPD_DC`                                         | E-Paper data/command select                                                  |
| 38   | `GPIO_EPD_RES`        | `EPD_RES`                                        | E-Paper reset                                                                |
| 44   | `GPIO_EPD_CS`         | `EPD_CS`                                         | E-Paper chip select                                                          |

### **ESP32-C3 Pin Assignments**

| GPIO | Physical Name   | Functional Name(s) | Purpose                     |
|------|-----------------|--------------------|-----------------------------|
| 0    | `GPIO_BUTTON_1` | *(future)*         | Available for button        |
| 1    | `GPIO_BUTTON_2` | *(future)*         | Available for button        |
| 2    | `GPIO_EPD_BUSY` | `EPD_BUSY`         | E-Paper busy signal         |
| 3    | `GPIO_EPD_CS`   | `EPD_CS`           | E-Paper chip select         |
| 4    | `GPIO_EPD_SCK`  | `EPD_SCK`          | E-Paper SPI clock           |
| 5    | `GPIO_BUTTON_3` | *(future)*         | Available for button        |
| 6    | `GPIO_EPD_SDI`  | `EPD_SDI`          | E-Paper SPI data (MOSI)     |
| 8    | `GPIO_EPD_RES`  | `EPD_RES`          | E-Paper reset               |
| 9    | `GPIO_EPD_DC`   | `EPD_DC`           | E-Paper data/command select |

---

## Benefits of Dual-Layer Strategy

### ✅ **Clarity at Both Levels**

- **Physical Layer:** Easy to identify actual GPIO numbers when debugging hardware
- **Functional Layer:** Code is self-documenting and readable

### ✅ **Hardware Debugging**

- When measuring with oscilloscope or multimeter, you can immediately see `GPIO_NUM_2`
- When reviewing code, you see `BUTTON_HALF_AND_HALF` for clear context

### ✅ **Centralized Configuration**

- All pin definitions in one file: `include/config/pins.h`
- No more scattered hardcoded GPIO numbers
- Single source of truth

### ✅ **Board Portability**

- Same functional names across different boards
- Hardware changes only require updating Layer 1
- Application code using Layer 2 doesn't need changes

### ✅ **Future Flexibility**

- ESP32-C3 button support can be added by simply mapping functional names to physical pins
- Easy to add new peripherals with clear naming

### ✅ **Shared Pin Documentation**

- GPIO 2 multi-function clearly documented:
    - `BUTTON_HALF_AND_HALF` (short press)
    - `BUTTON_FACTORY_RESET` (long press)

---

## Compilation Results

### ✅ **ESP32-S3 Build**

- **Status:** SUCCESS
- **RAM Usage:** 30.0% (98,200 / 327,680 bytes)
- **Flash Usage:** 45.0% (1,416,297 / 3,145,728 bytes)
- **Build Time:** 43.29 seconds

### ✅ **ESP32-C3 Build**

- **Status:** SUCCESS
- **RAM Usage:** 29.5% (96,644 / 327,680 bytes)
- **Flash Usage:** 48.9% (1,538,610 / 3,145,728 bytes)
- **Build Time:** 38.73 seconds

**No compilation errors or warnings related to GPIO refactoring.**

---

## Files Modified

1. ✅ `include/config/pins.h` - Complete restructure with dual-layer naming
2. ✅ `src/util/battery_manager.cpp` - Use centralized `Pins::BATTERY_ADC` and `Pins::ADC_EN`
3. ✅ `include/util/factory_reset.h` - Remove hardcoded GPIO, include `pins.h`
4. ✅ `src/util/factory_reset.cpp` - Use `Pins::BUTTON_FACTORY_RESET` with board guards

---

## Usage Guidelines

### **For Application Code:**

Always use **Layer 2 functional names**:

```cpp
pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
digitalRead(Pins::BUTTON_WEATHER_ONLY);
digitalWrite(Pins::ADC_EN, HIGH);
```

### **For Hardware Documentation:**

Reference **Layer 1 physical names** when documenting hardware connections:

```cpp
// Connect button to GPIO_BUTTON_1 (GPIO 2)
// Connect battery ADC to GPIO_BATTERY_ADC (GPIO 1)
```

### **Adding New Pins:**

1. Define in Layer 1 with `GPIO_` prefix
2. Create functional alias in Layer 2
3. Add to pin mapping table in documentation

---

## Future Enhancements

### **ESP32-C3 Button Support**

When ready to add buttons to ESP32-C3:

```cpp
#ifdef BOARD_ESP32_C3
    constexpr int BUTTON_MODE_1 = GPIO_BUTTON_1;
    constexpr int BUTTON_MODE_2 = GPIO_BUTTON_2;
    constexpr int BUTTON_MODE_3 = GPIO_BUTTON_3;
#endif
```

Physical GPIOs (0, 1, 5) are already defined and ready for use.

---

## Conclusion

The GPIO pin refactoring successfully implements a clean dual-layer naming strategy that:

- ✅ Eliminates confusion between physical and logical names
- ✅ Centralizes all pin definitions
- ✅ Makes code more maintainable
- ✅ Improves documentation
- ✅ Prepares for future hardware expansion
- ✅ Compiles successfully on both ESP32-S3 and ESP32-C3

**The codebase now has a clear, consistent, and well-documented GPIO pin management system.**

