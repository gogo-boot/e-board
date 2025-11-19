# Button Controls (ESP32-S3 Only)

MyStation can be controlled with physical buttons when using an ESP32-S3 board. This feature allows you to temporarily
change the display mode without accessing the web interface.

> ⚠️ **Hardware Requirement**: Button controls are **only available on ESP32-S3** boards. The ESP32-C3 Super Mini does
> not have button support in the current configuration.

## Button Overview

### Available Buttons

| Button   | GPIO | Function             | Display Mode                      |
|----------|------|----------------------|-----------------------------------|
| Button 1 | 2    | Half & Half Mode     | Weather + Departures split screen |
| Button 2 | 3    | Weather Only Mode    | Full screen weather display       |
| Button 3 | 5    | Departures Only Mode | Full screen departure display     |

### Additional Function

**Button 1** also serves as the Factory Reset button:

- **Short press** (< 1 second): Switch to Half & Half display mode
- **Long press** (5 seconds during boot): Initiate factory reset

## Hardware Setup

### Wiring Buttons

Each button connects between a GPIO pin and Ground (GND):

```
Button Wiring Diagram:

ESP32-S3          Push Button         Ground
─────────         ───────────         ──────

GPIO 2  ──────────┤       ├──────────  GND
                  └───────┘

GPIO 3  ──────────┤       ├──────────  GND
                  └───────┘

GPIO 5  ──────────┤       ├──────────  GND
                  └───────┘
```

### Button Specifications

**Button Type**: Momentary push button (normally open)

- Press to connect GPIO to GND
- Release to disconnect

**Pull-up Resistors**: Internal (no external resistor needed)

- GPIO configured with internal pull-up in software
- Button pressed = LOW signal
- Button released = HIGH signal

**Debouncing**: Implemented in software

- 50ms debounce delay
- Prevents false triggers

## How Buttons Work

### Normal Operation Mode

When your MyStation is operating normally:

1. **Device wakes** from deep sleep at scheduled time
2. **Checks for button press** during wake-up
3. If no button: Uses configured display mode
4. If button pressed: Switches to temporary mode
5. **Fetches data** from APIs
6. **Updates display** with selected mode
7. **Returns to sleep** until next scheduled wake

### Button Press Behavior

When you press a button while device is sleeping:

1. **ESP32 wakes up** immediately from deep sleep
2. **Detects which button** triggered the wake-up
3. **Enters temporary mode** with selected display style
4. **Connects to WiFi** (2.4 GHz network)
5. **Fetches fresh data** (weather and/or departures)
6. **Updates display** showing requested information
7. **Stays awake** for 2 minutes
8. **Returns to normal** operation after timeout

### Temporary Mode

**Duration**: 2 minutes (120 seconds)

During temporary mode:

- ✅ Buttons remain active (you can press again)
- ✅ Display shows selected mode
- ✅ Can switch between modes freely
- ✅ Fresh data is fetched
- ⏰ After 2 minutes: Returns to configured mode

**Why temporary?**

- Saves battery life
- Prevents accidentally staying in wrong mode
- Normal schedule resumes automatically

## Using the Buttons

### Check Current Weather

**Press Button 2** (Weather Only Mode)

Display shows:

- Current temperature and "feels like"
- Weather icon and description
- 24-hour temperature graph
- Wind, humidity, precipitation
- Sunrise/sunset times

**Use case**: Quick weather check before leaving house

### Check Departures

**Press Button 3** (Departures Only Mode)

Display shows:

- List of upcoming departures
- Departure times with delays
- Platform/track information
- Transport type (RE, S-Bahn, Bus, etc.)
- Destination station

**Use case**: Check when next train/bus arrives

### Check Both

**Press Button 1** (Half & Half Mode)

Display shows:

- Top half: Weather summary
- Bottom half: Departure list
- Balanced view of both information types

**Use case**: See everything at once

### Factory Reset

**Press and hold Button 1 during power-on** for 5 seconds

See [Factory Reset Guide](factory-reset.md) for detailed instructions.

## Display Modes Explained

### Mode 1: Half & Half (Button 1)

**Layout**:

```
┌─────────────────────────────┐
│ Weather Information         │
│ Temperature, Icon, Forecast │
│                             │
├─────────────────────────────┤
│ Departure Information       │
│ Next trains/buses           │
│                             │
└─────────────────────────────┘
```

**Best for**: Balanced information, daily use

### Mode 2: Weather Only (Button 2)

**Layout**:

```
┌─────────────────────────────┐
│                             │
│   Large Weather Display     │
│   Temperature Graph         │
│   Detailed Forecast         │
│   Wind, Humidity, etc.      │
│                             │
└─────────────────────────────┘
```

**Best for**: Weather enthusiasts, outdoor planning

### Mode 3: Departures Only (Button 3)

**Layout**:

```
┌─────────────────────────────┐
│                             │
│   Departure List            │
│   More departures visible   │
│   Detailed timing info      │
│   Platform numbers          │
│                             │
└─────────────────────────────┘
```

**Best for**: Commuters, catching transport

## Button Behavior Examples

### Example 1: Morning Commute

**Scenario**: Need to know when next S-Bahn departs

1. **Walk past display** showing yesterday's weather
2. **Press Button 3** (Departures Only)
3. **Device wakes**, connects to WiFi
4. **Fresh departures** appear in 30-40 seconds
5. **Check next train** and leave accordingly
6. **After 2 minutes**: Returns to normal schedule

### Example 2: Planning Weekend

**Scenario**: Want detailed weather forecast

1. **Press Button 2** (Weather Only)
2. **Device fetches** latest weather data
3. **See full forecast** with temperature graph
4. **Press Button 3** if also need transport info
5. **Device immediately** switches modes
6. **After 2 minutes idle**: Returns to configured mode

### Example 3: Quick Check

**Scenario**: Just want to see current status

1. **Press Button 1** (Half & Half)
2. **See both** weather and departures
3. **Get overview** of day ahead
4. **Walk away** - mode resets automatically

## Technical Details

### Wake-up Detection

ESP32 can wake from deep sleep via button press:

```cpp
// Buttons configured as EXT0/EXT1 wake sources
// Any button press triggers immediate wake-up
// Boot code detects which button was pressed
```

### Mode Persistence

**Temporary Mode**:

- Display mode change is temporary
- Original configured mode stored
- Automatic return after timeout

**Permanent Mode Change**:

- Use web interface to change default mode
- Access `http://mystation.local`
- Select preferred mode
- Click "Save Settings"

### Button Debouncing

Software debouncing prevents false triggers:

- 50ms delay after button detected
- Filters mechanical bounce
- Ensures clean button detection

### Power Consumption

Button wake-up and temporary mode:

- Wake from sleep: ~5 seconds
- WiFi connection: ~10 seconds
- Data fetch: ~10 seconds
- Display update: ~30 seconds
- **Total active time**: ~55 seconds
- **Battery impact**: Minimal (equivalent to one scheduled update)

## Troubleshooting Buttons

### Buttons Don't Work

**Check**:

1. ✅ Using ESP32-S3 (buttons not supported on C3)
2. ✅ Buttons wired correctly (GPIO to GND)
3. ✅ Button type is momentary (normally open)
4. ✅ Firmware built with `BOARD_ESP32_S3` defined

See [Troubleshooting Guide](troubleshooting.md#button-issues-esp32-s3-only) for detailed solutions.

### Wrong Mode Displays

**Possible causes**:

- Buttons wired to different GPIOs than expected
- Need to update `pins.h` to match your wiring
- Check serial monitor to see which GPIO detected

### Button Works Once Then Stops

**Likely causes**:

- Button hardware issue (stuck, damaged)
- Wiring problem (loose connection)
- Software issue (check serial monitor for errors)

### Factory Reset Activates Unintentionally

**Solution**:

- Don't hold Button 1 during power-on unless you want reset
- Release button quickly after power-on
- Full 5-second hold required for reset

## Button Hardware Tips

### Recommended Button Types

✅ **Good choices**:

- Tactile push buttons (6x6mm, 12x12mm)
- Panel-mount momentary switches
- Any momentary SPST switch

❌ **Not suitable**:

- Toggle switches (maintain state)
- Latching buttons
- Capacitive touch (different circuit needed)

### Installation Tips

1. **Enclosure mounting**:
    - Drill holes for panel-mount buttons
    - Label buttons clearly
    - Position for easy access

2. **Wire length**:
    - Keep wires short (reduces noise)
    - 10-20cm typically sufficient
    - Use stranded wire for flexibility

3. **Strain relief**:
    - Secure wires near buttons
    - Prevent stress on solder joints
    - Use hot glue or cable ties

## Without Buttons (ESP32-C3)

If you're using ESP32-C3 without buttons:

### Alternative Control Methods

1. **Web Interface**:
    - Access `http://mystation.local`
    - Change display mode in settings
    - Click "Save" to apply

2. **Scheduled Mode Changes**:
    - Configure different modes for different times
    - Example: Weather in morning, departures in evening
    - Requires custom firmware modification

3. **Upgrade to ESP32-S3**:
    - Add button support
    - Similar form factor
    - Compatible pin layout

## Related Documentation

- 🏠 [User Guide Home](index.md)
- 🔧 [Hardware Assembly](hardware-assembly.md) - Button wiring instructions
- 🔄 [Factory Reset](factory-reset.md) - Using Button 1 for reset
- 📋 [Troubleshooting](troubleshooting.md) - Button issues and solutions

---

**Note**: Button functionality is a convenience feature. Your MyStation works perfectly fine without buttons using the
configured display mode and web interface for control.

