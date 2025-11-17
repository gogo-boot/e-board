# Button Quick Reference

## GPIO Pin Mapping (ESP32-S3 Only)

| Button   | GPIO | Function       | Display Mode                     |
|----------|------|----------------|----------------------------------|
| Button 1 | 2    | Half & Half    | Weather + Departure split screen |
| Button 2 | 3    | Weather Only   | Full screen weather display      |
| Button 3 | 5    | Departure Only | Full screen departure display    |

## Wiring Diagram

```
ESP32-S3          Push Button         Power
─────────         ───────────         ─────

GPIO 2  ──────────┤       ├──────────  GND
                  └───────┘

GPIO 3  ──────────┤       ├──────────  GND
                  └───────┘

GPIO 5  ──────────┤       ├──────────  GND
                  └───────┘

Note: Internal pull-up resistors are enabled in software
      Button pressed = LOW signal
      Button released = HIGH signal
```

## Behavior Summary

### Normal Mode

- Device operates with configured display mode
- Wakes on schedule (weather interval, transport interval)
- Buttons monitored during deep sleep

### Temporary Mode (After Button Press)

- Shows requested display mode immediately
- Fetches fresh data
- Stays in temporary mode for **2 minutes**
- Buttons remain active (can switch modes)
- Auto-returns to configured mode after timeout

## Constants

```cpp
// Defined in ButtonManager class
TEMPORARY_MODE_TIMEOUT = 120 seconds  (2 minutes)
DEBOUNCE_DELAY_MS = 50 milliseconds
```

## Code Examples

### Check Button in Your Code

```cpp
#include "util/button_manager.h"

// Initialize buttons (call once in setup)
ButtonManager::init();

// Check if button is currently pressed
if (ButtonManager::isButtonPressed(Pins::BUTTON_WEATHER_ONLY)) {
    // Button 2 is pressed
}

// Check if device woke from button press
if (ButtonManager::wasWokenByButton()) {
    int8_t mode = ButtonManager::getWakeupButtonMode();
    // mode = DISPLAY_MODE_WEATHER_ONLY, etc.
}
```

### Enable Button Wakeup Before Sleep

```cpp
#include "util/sleep_utils.h"

// Sleep for 5 minutes with button wakeup
enterDeepSleepWithButtonWakeup(300);  // seconds
```

## Display Mode Values

```cpp
#define DISPLAY_MODE_HALF_AND_HALF  0
#define DISPLAY_MODE_WEATHER_ONLY   1
#define DISPLAY_MODE_DEPARTURE_ONLY 2
```

## Troubleshooting One-Liners

| Problem                       | Solution                                                     |
|-------------------------------|--------------------------------------------------------------|
| Button doesn't wake device    | Check wiring: button should connect GPIO to GND when pressed |
| Wrong mode displayed          | Verify GPIO number matches button in pins.h                  |
| Not returning to original     | Ensure NTP time is synced (check WiFi connection)            |
| Button works once then stops  | Check for button hardware issue (stuck/bouncing)             |
| No button support on my board | Feature is ESP32-S3 only (disabled on ESP32-C3)              |

## Hardware Requirements

- **Buttons**: Momentary tactile switches (normally open)
- **Recommended**: 6mm x 6mm tactile switches
- **Voltage**: 3.3V compatible
- **Current**: < 1mA (pull-up resistor ~10kΩ internal)
- **Debounce**: Handled in software (50ms)

## Board Compatibility

| Board    | Support | Notes                                   |
|----------|---------|-----------------------------------------|
| ESP32-S3 | ✅ Yes   | Full functionality                      |
| ESP32-C3 | ❌ No    | Feature disabled (different GPIO usage) |
| ESP32    | ❌ No    | Not tested, may work with pin changes   |

## Quick Test Procedure

1. **Upload firmware** to ESP32-S3
2. **Wire three buttons** to GPIO 2, 3, 5 (and GND)
3. **Let device complete** one normal cycle
4. **Press Button 2** (GPIO 3)
5. **Verify** weather-only mode shows
6. **Wait 2 minutes**
7. **Verify** returns to configured mode
8. **Done!** ✅

