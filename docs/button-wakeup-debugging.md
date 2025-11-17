# Button Wakeup Debugging Guide

## Issue: Buttons Not Waking Device from Deep Sleep

### Current Configuration

- **Button 1**: GPIO 2 → DISPLAY_MODE_HALF_AND_HALF
- **Button 2**: GPIO 3 → DISPLAY_MODE_WEATHER_ONLY
- **Button 3**: GPIO 4 → DISPLAY_MODE_DEPARTURE_ONLY

### Diagnostic Features Added

#### 1. Wakeup Cause Logging

The firmware now logs the wakeup cause at boot:

```
=== Wakeup Cause: X ===
```

Expected values:

- `0` (ESP_SLEEP_WAKEUP_UNDEFINED) = Power on/reset
- `2` (ESP_SLEEP_WAKEUP_EXT0) = Single GPIO wakeup
- `3` (ESP_SLEEP_WAKEUP_EXT1) = Multiple GPIO wakeup **← EXPECTED FOR BUTTONS**
- `4` (ESP_SLEEP_WAKEUP_TIMER) = Timer wakeup

#### 2. RTC GPIO Validation

At init, the firmware checks if each button GPIO supports RTC:

```
[BUTTON] ✓ GPIO X supports RTC (EXT1 wakeup)
[BUTTON] ✗ GPIO X does NOT support RTC - cannot wake from deep sleep!
```

#### 3. Button State Monitoring

Shows current button states at init:

```
[BUTTON] Current button states:
[BUTTON]   GPIO 2: HIGH (not pressed) / LOW (PRESSED!)
[BUTTON]   GPIO 3: HIGH (not pressed) / LOW (PRESSED!)
[BUTTON]   GPIO 4: HIGH (not pressed) / LOW (PRESSED!)
```

#### 4. EXT1 Wakeup Configuration Details

Shows exactly what's being configured:

```
[BUTTON] Enabling EXT1 wakeup for buttons...
[BUTTON]   Button mask: 0xXXX
[BUTTON]   GPIO X bit: 1
[BUTTON] ✓ EXT1 wakeup enabled successfully
```

OR

```
[BUTTON] ✗ Failed to enable EXT1 wakeup! Error: 0xXXX
[BUTTON]   This usually means one or more GPIOs don't support RTC!
```

### GPIO Diagnostic Test Mode

To identify which GPIOs your buttons are actually connected to:

#### Enable Test Mode

1. Open `src/main.cpp`
2. Find line ~150: `// #define RUN_GPIO_DIAGNOSTIC_TEST`
3. **Uncomment it**: `#define RUN_GPIO_DIAGNOSTIC_TEST`
4. Build and upload firmware
5. Open serial monitor

The device will:

- Scan all GPIOs 0-21 for 30 seconds
- Report which GPIOs support RTC
- Detect and log every button press with GPIO number
- Restart automatically after test

Example output:

```
╔════════════════════════════════════════════════╗
║   GPIO BUTTON DIAGNOSTIC TEST                  ║
║   Press each button to identify GPIO numbers   ║
║   Duration: 30 seconds                         ║
╚════════════════════════════════════════════════╝

Testing GPIOs: 0-21
  GPIO  0: ✓ RTC supported
  GPIO  1: ✓ RTC supported
  GPIO  2: ✓ RTC supported
  GPIO  3: ✓ RTC supported
  GPIO  4: ✓ RTC supported
  ...

>>> PRESS YOUR BUTTONS NOW <<<

🔘 BUTTON PRESSED on GPIO 2 [RTC: ✓]
🔘 BUTTON PRESSED on GPIO 3 [RTC: ✓]
🔘 BUTTON PRESSED on GPIO 4 [RTC: ✓]
```

After identifying the correct GPIOs, update `include/config/pins.h` and **comment out** the diagnostic test.

### Common Issues & Solutions

#### Issue 1: GPIOs Don't Support RTC

**Symptom:**

```
[BUTTON] ✗ GPIO X does NOT support RTC - cannot wake from deep sleep!
```

**Solution:**

- Use different GPIOs that support RTC
- On ESP32-S3, RTC GPIOs are: 0-21
- Recommended safe GPIOs: 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14

**⚠️ Avoid these GPIOs:**

- 19, 20 (USB)
- 26-48 (May not support RTC on some variants)

#### Issue 2: EXT1 Wakeup Fails to Enable

**Symptom:**

```
[BUTTON] ✗ Failed to enable EXT1 wakeup! Error: 0x102
```

**Cause:**

- One or more GPIOs in the button mask don't support RTC
- GPIO is being used for another purpose (SPI, I2C, etc.)

**Solution:**

1. Run GPIO diagnostic test to verify which GPIOs work
2. Check `pins.h` for GPIO conflicts
3. Use only RTC-capable GPIOs

#### Issue 3: GPIO Conflict with E-Paper Display

**Current Conflict:**

- GPIO 4 is used for both `EPD_BUSY` and `BUTTON_DEPARTURE_ONLY`

**This WILL cause problems!**

**Solution Options:**
a) **Use different button GPIO** (recommended):

- Change `BUTTON_DEPARTURE_ONLY` to GPIO 5, 8, or 9

b) **Reconfigure E-Paper pins**:

- Move EPD_BUSY to a different GPIO

#### Issue 4: Wakeup Cause Shows TIMER, Not EXT1

**Symptom:**

```
=== Wakeup Cause: 4 ===
  → Timer wakeup
```

**Causes:**

1. Buttons not pressed during sleep
2. EXT1 not properly enabled
3. GPIO doesn't support RTC
4. Button wiring issue

**Debug Steps:**

1. Check if EXT1 enable succeeded:
   ```
   [BUTTON] ✓ EXT1 wakeup enabled successfully
   ```
2. Verify button wiring (GPIO to GND when pressed)
3. Run diagnostic test to confirm buttons work
4. Try holding button during entire sleep period

#### Issue 5: No Wakeup at All

**Symptom:**

- Device doesn't wake when button pressed
- Serial monitor shows no activity

**Debug Steps:**

1. Verify device actually went to sleep:
   ```
   [SLEEP] Entering deep sleep for X seconds with button wakeup enabled
   ```

2. Check button is triggering LOW on GPIO:
    - Use multimeter to verify button closes circuit
    - Test GPIO reads LOW when button pressed

3. Verify correct GPIO numbers:
    - Run diagnostic test
    - Compare with your hardware

4. Check RTC GPIO support:
    - Look for ✓ marks in init log

### Recommended Testing Procedure

1. **First Boot - Enable Diagnostics**
   ```cpp
   #define RUN_GPIO_DIAGNOSTIC_TEST  // in main.cpp
   ```

2. **Upload and Monitor Serial**
   ```bash
   pio run -e esp32-s3-base -t upload && pio device monitor -e esp32-s3-base
   ```

3. **Press Each Button During Test**
    - Note which GPIO numbers are reported
    - Verify all show "[RTC: ✓]"

4. **Update Pin Configuration**
    - Edit `include/config/pins.h`
    - Set correct GPIO numbers
    - Verify no conflicts with other pins

5. **Disable Diagnostics**
   ```cpp
   // #define RUN_GPIO_DIAGNOSTIC_TEST
   ```

6. **Test Normal Operation**
    - Upload firmware
    - Let device complete one cycle and sleep
    - Press a button
    - Verify wakeup cause = EXT1

7. **Monitor Logs**
   ```
   [MAIN] === Wakeup Cause: 3 ===
   [MAIN]   → EXT1 (multiple GPIO RTC_CNTL)
   [MAIN]   → Wakeup pin mask: 0xXX
   [BUTTON] Woken by BUTTON_XXX (GPIO X)
   ```

### ESP32-S3 RTC GPIO Reference

| GPIO  | RTC Support | Typical Use | Safe for Button?     |
|-------|-------------|-------------|----------------------|
| 0     | ✓ Yes       | Boot mode   | ⚠️ Use with caution  |
| 1     | ✓ Yes       | ADC         | ✓ Yes                |
| 2     | ✓ Yes       | ADC         | ✓ Yes                |
| 3     | ✓ Yes       | ADC         | ✓ Yes                |
| 4     | ✓ Yes       | ADC         | ⚠️ Used for EPD_BUSY |
| 5     | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 6     | ✓ Yes       | -           | ⚠️ Used for ADC_EN   |
| 7     | ✓ Yes       | -           | ⚠️ Used for EPD_SCK  |
| 8     | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 9     | ✓ Yes       | -           | ⚠️ Used for EPD_SDI  |
| 10    | ✓ Yes       | -           | ⚠️ Used for EPD_DC   |
| 11    | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 12    | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 13    | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 14    | ✓ Yes       | -           | ✓ Yes (Recommended)  |
| 15-21 | ✓ Yes       | Various     | Check conflicts      |

### Suggested Button GPIO Assignment

To avoid conflicts with existing hardware:

**Option 1: Use GPIO 11, 12, 13**

```cpp
constexpr int BUTTON_HALF_AND_HALF = 11;
constexpr int BUTTON_WEATHER_ONLY = 12;
constexpr int BUTTON_DEPARTURE_ONLY = 13;
```

**Option 2: Use GPIO 5, 8, 11**

```cpp
constexpr int BUTTON_HALF_AND_HALF = 5;
constexpr int BUTTON_WEATHER_ONLY = 8;
constexpr int BUTTON_DEPARTURE_ONLY = 11;
```

**Current Assignment (Has Conflict!):**

```cpp
constexpr int BUTTON_HALF_AND_HALF = 2;    // OK
constexpr int BUTTON_WEATHER_ONLY = 3;     // OK
constexpr int BUTTON_DEPARTURE_ONLY = 4;   // ⚠️ CONFLICTS WITH EPD_BUSY!
```

### Next Steps

1. ✅ Upload firmware with diagnostics enabled
2. ✅ Run GPIO scan test and note button GPIOs
3. ✅ Check serial logs for RTC support confirmation
4. ✅ Resolve GPIO 4 conflict (use different GPIO)
5. ✅ Update pins.h with correct values
6. ✅ Test EXT1 wakeup functionality
7. ✅ Verify button operations in normal mode

### Serial Monitor Commands

```bash
# Upload and monitor in one command
pio run -e esp32-s3-base -t upload && pio device monitor -e esp32-s3-base

# Just monitor (after upload)
pio device monitor -e esp32-s3-base

# Build only
pio run -e esp32-s3-base
```

### Expected Working Log Sequence

```
[MAIN] System starting...
[MAIN] === Wakeup Cause: 0 ===
[MAIN]   → Power on or reset (not from deep sleep)
[BATTERY] Initializing battery manager...
[BUTTON] Initializing button manager...
[BUTTON] Button pins configured: GPIO 11, 12, 13
[BUTTON] Checking RTC GPIO support...
[BUTTON] ✓ GPIO 11 supports RTC (EXT1 wakeup)
[BUTTON] ✓ GPIO 12 supports RTC (EXT1 wakeup)
[BUTTON] ✓ GPIO 13 supports RTC (EXT1 wakeup)
[BUTTON] Current button states:
[BUTTON]   GPIO 11: HIGH (not pressed)
[BUTTON]   GPIO 12: HIGH (not pressed)
[BUTTON]   GPIO 13: HIGH (not pressed)
...
[SLEEP] Entering deep sleep for 180 seconds with button wakeup enabled
[BUTTON] Enabling EXT1 wakeup for buttons...
[BUTTON]   Button mask: 0x3800
[BUTTON]   GPIO 11 bit: 1
[BUTTON]   GPIO 12 bit: 1
[BUTTON]   GPIO 13 bit: 1
[BUTTON] ✓ EXT1 wakeup enabled successfully (mask: 0x3800)

--- BUTTON PRESSED ---

[MAIN] System starting...
[MAIN] === Wakeup Cause: 3 ===
[MAIN]   → EXT1 (multiple GPIO RTC_CNTL)
[MAIN]   → Wakeup pin mask: 0x800
[BUTTON] Woken by BUTTON_HALF_AND_HALF (GPIO 11)
[MAIN] Woken by button press! Temporary display mode: 0
```

