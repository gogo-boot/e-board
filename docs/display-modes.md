# Display Modes Documentation

This document provides comprehensive information about the MyStation E-Board display system, including modes, orientations, and usage patterns.

## Table of Contents

- [Overview](#overview)
- [Display Hardware](#display-hardware)
- [Orientations](#orientations)
- [Display Modes](#display-modes)
- [Partial Updates](#partial-updates)
- [Content Adaptation](#content-adaptation)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)

## Overview

The MyStation E-Board uses a flexible display manager system that automatically adapts content layout based on:
- Available data (weather, departures, or both)
- Screen orientation (portrait or landscape)
- Display mode (split-screen or full-screen)
- Update frequency requirements

## Display Hardware

### Supported E-Paper Display
- **Model**: GoodDisplay GDEY075T7
- **Size**: 7.5 inches
- **Resolution**: 800×480 pixels (automatically detected)
- **Colors**: Black and white
- **Refresh**: ~2 seconds full, ~0.5 seconds partial
- **Power**: Ultra-low power consumption

### Supported Microcontrollers

#### ESP32-C3 Super Mini (Current)
- **CPU**: RISC-V single-core @ 160MHz
- **RAM**: 400KB SRAM
- **Flash**: 4MB
- **WiFi**: 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **Power**: 3.3V operation
- **Size**: 27×13mm
- **Cost**: ~$3-5

#### XIAO ESP32-C6 (Recommended Upgrade)
- **CPU**: RISC-V dual-core @ 160MHz
- **RAM**: 512KB SRAM (25% more)
- **Flash**: 4MB
- **WiFi**: 802.11 a/b/g/n (WiFi 6 support)
- **Bluetooth**: BLE 5.0 + Zigbee/Thread
- **Power**: 3.3V operation
- **Size**: 21×17.5mm (more compact)
- **Cost**: ~$5-7
- **Advantages**: 
  - Better WiFi performance and range
  - More RAM for complex operations
  - Zigbee/Thread IoT protocols
  - Better power efficiency
  - Smaller form factor

### Pin Connections

#### ESP32-C3 Super Mini
```
ESP32-C3 Pin    E-Paper Pin    Function
────────────    ───────────    ────────
GPIO 2          BUSY           Status signal
GPIO 3          CS             Chip select
GPIO 4          SCK            SPI clock
GPIO 6          SDI/MOSI       SPI data
GPIO 8          RES            Reset
GPIO 9          DC             Data/Command
3.3V            VCC            Power supply
GND             GND            Ground
```

#### XIAO ESP32-C6 (Alternative)
```
XIAO C6 Pin     E-Paper Pin    Function
───────────     ───────────    ────────
GPIO 2          BUSY           Status signal
GPIO 3          CS             Chip select
GPIO 4          SCK            SPI clock
GPIO 5          SDI/MOSI       SPI data
GPIO 6          RES            Reset
GPIO 7          DC             Data/Command
3.3V            VCC            Power supply
GND             GND            Ground
```

### Power Consumption Comparison

#### ESP32-C3 Super Mini
```
Mode              Current    Notes
────────────────────────────────────
Active (WiFi)     ~80mA     During API calls
Deep Sleep        ~50μA     Between updates
Display Update    ~25mA     E-paper refresh
```

#### XIAO ESP32-C6 (Improved)
```
Mode              Current    Notes
────────────────────────────────────
Active (WiFi)     ~70mA     Better power efficiency (-12%)
Deep Sleep        ~35μA     Improved sleep modes (-30%)
Display Update    ~25mA     Same e-paper consumption
```

### Battery Life Impact (ESP32-C6 vs C3)

**2500mAh Battery with 5-minute updates:**

| Microcontroller | Battery Life | Total Refreshes | Improvement |
|-----------------|--------------|-----------------|-------------|
| ESP32-C3        | 75 days      | 21,588         | Baseline    |
| XIAO ESP32-C6   | 85 days      | 24,480         | +13% life   |

**1000mAh Battery with 10-minute updates:**

| Microcontroller | Battery Life | Total Refreshes | Improvement |
|-----------------|--------------|-----------------|-------------|
| ESP32-C3        | 57 days      | 8,220          | Baseline    |
| XIAO ESP32-C6   | 65 days      | 9,360          | +14% life   |

## Orientations

### Landscape Mode (Default)
- **Physical**: Display is wider than tall (native orientation)
- **Resolution**: 800×480 pixels
- **Split**: Vertical (left/right halves)
- **Weather Area**: Left half (400×480 px)
- **Departure Area**: Right half (400×480 px)
- **Best for**: Desk placement, natural reading, wide viewing

### Portrait Mode
- **Physical**: Display is taller than wide (rotated 90°)
- **Resolution**: 480×800 pixels
- **Split**: Horizontal (top/bottom halves)
- **Weather Area**: Top half (480×400 px)
- **Departure Area**: Bottom half (480×400 px)
- **Best for**: Wall mounting, narrow spaces, vertical mounting

## Display Modes

### 1. Half-and-Half Mode

**Purpose**: Show both weather and transport information simultaneously

**Behavior**:
- If both data available: Shows split-screen layout
- If only weather available: Shows weather in designated half
- If only departures available: Shows departures in designated half

**Landscape Layout Example (Default)**:
```
┌──────────────────┬──────────────────┐
│  🌤️ Weather      │  🚌 Departures   │
│                  │                  │
│  22°C            │  Frankfurt Hbf   │
│  Frankfurt       │                  │
│  Partly Cloudy   │  S1   Wiesbaden  │
│                  │  14:23   Pl. 3   │
│  High: 25°C      │                  │
│  Low: 15°C       │  RE1  Fulda      │
│                  │  14:25   Pl. 7   │
│  Next Hours:     │                  │
│  14:00  23°C     │  Bus  Sachsenh.  │
│  15:00  24°C     │  14:27   Stop A  │
│  16:00  25°C     │                  │
│                  │  Updated: 23s    │
└──────────────────┴──────────────────┘
```

**Portrait Layout Example**:
```
┌─────────────────────────────────────┐
│           🌤️ Weather               │
│                                     │
│     22°C Frankfurt Partly Cloudy   │
│     High: 25°C  Low: 15°C          │
│                                     │
│     Next Hours:                     │
│     14:00  23°C    15:00  24°C     │
│     16:00  25°C    17:00  26°C     │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│           🚌 Departures             │
│                                     │
│         Frankfurt Hauptbahnhof     │
│                                     │
│  Line  Destination      Time  Plat │
│  ──────────────────────────────────│
│  S1    Wiesbaden       14:23   3   │
│  RE1   Fulda           14:25   7   │
│  Bus61 Sachsenhausen   14:27   A   │
│                                     │
│              Updated: 23s ago       │
└─────────────────────────────────────┘
```

### 2. Weather-Only Mode

**Purpose**: Detailed weather information with extended forecast

**Features**:
- Large temperature display
- Current conditions
- High/low temperatures
- 6-hour forecast
- Sunrise/sunset times
- Location information

**Layout Example**:
```
┌────────────────────────────────────┐
│        🌤️ Weather Information      │
│                                    │
│        22°C    Frankfurt           │
│        Partly Cloudy               │
│                                    │
│    High: 25°C    Low: 15°C         │
│                                    │
│    Next Hours:                     │
│    ──────────────────────────────  │
│    14:00    23°C    10% rain       │
│    15:00    24°C     5% rain       │
│    16:00    25°C     0% rain       │
│    17:00    24°C     0% rain       │
│    18:00    23°C     5% rain       │
│    19:00    22°C    15% rain       │
│                                    │
│    ☀️↑ 06:30        ☀️↓ 20:45      │
└────────────────────────────────────┘
```

### 3. Departures-Only Mode

**Purpose**: Comprehensive departure board with maximum transport information

**Features**:
- Station name
- Up to 15 departures
- Line numbers
- Destinations
- Departure times (real-time when available)
- Track/platform information
- Transport type indicators

**Layout Example**:
```
┌────────────────────────────────────┐
│         🚌 Departure Board         │
│                                    │
│      Frankfurt Hauptbahnhof       │
│                                    │
│ Line    Destination         Time   │
│ ──────────────────────────────────│
│ S1      Wiesbaden          14:23   │
│ RE1     Fulda              14:25   │
│ S8      Hanau              14:26*  │
│ Bus61   Sachsenhausen      14:27   │
│ U4      Bockenheimer W.    14:28   │
│ RB15    Bad Hersfeld       14:30   │
│ S3      Bad Soden          14:32   │
│ RE50    Limburg            14:35   │
│ S1      Rödelheim          14:38   │
│ Bus36   Niederrad          14:40   │
│                                    │
│ * = Real-time data                 │
│ Updated: 23 seconds ago            │
└────────────────────────────────────┘
```

## Partial Updates

### Benefits
- **Faster refresh**: ~0.5s vs 2s full screen
- **Lower power consumption**: Less display driver activity
- **Better user experience**: No full screen flicker
- **Efficient data usage**: Update only changed information

### Update Strategies

#### Weather Updates (Every 10 minutes)
```cpp
// Update only weather half
DisplayManager::updateWeatherHalf(weatherData);
```

#### Departure Updates (Every 2 minutes)
```cpp
// Update only departure half
DisplayManager::updateDepartureHalf(departureData);
```

#### Full Updates (When needed)
```cpp
// Complete refresh (mode change, error recovery)
DisplayManager::displayHalfAndHalf(&weather, &departures);
```

### Technical Implementation
- Uses `setPartialWindow()` for specific screen regions
- Automatically redraws divider lines
- Preserves content in non-updated areas
- Handles boundary effects properly

## Content Adaptation

### Font Scaling

#### Large Font (18pt) - `FreeMonoBold18pt7b`
- **Usage**: Main titles, current temperature
- **Character width**: ~12-15 pixels
- **Line height**: ~25 pixels
- **Best for**: Primary information that needs immediate attention

#### Medium Font (12pt) - `FreeMonoBold12pt7b`
- **Usage**: Station names, location, important data
- **Character width**: ~8-10 pixels
- **Line height**: ~18 pixels
- **Best for**: Secondary information, labels

#### Small Font (9pt) - `FreeMonoBold9pt7b`
- **Usage**: Details, timestamps, forecast data
- **Character width**: ~6-7 pixels
- **Line height**: ~14 pixels
- **Best for**: Detailed information, compact layouts

### Text Truncation Rules

#### Half-Screen Mode
- **Station names**: Maximum 20 characters + "..."
- **Destinations**: Maximum 12 characters + "..."
- **Weather conditions**: Maximum 15 characters + "..."

#### Full-Screen Mode
- **Station names**: Maximum 30 characters + "..."
- **Destinations**: Maximum 18 characters + "..."
- **Weather conditions**: Full text displayed

### Layout Priorities

#### Weather Section Priority
1. Current temperature (largest)
2. Location/city name
3. Current condition
4. High/low temperatures
5. Short-term forecast
6. Sunrise/sunset (full screen only)

#### Departure Section Priority
1. Station name
2. Next 3-5 departures (immediate)
3. Line numbers and destinations
4. Real-time vs scheduled times
5. Platform/track information
6. Update timestamp

## Display Regions

The display manager supports precise control over different screen regions for partial updates and clearing operations.

### Region Types

#### Orientation-Specific Regions
- **`LEFT_HALF`**: Left half in landscape mode (weather area)
- **`RIGHT_HALF`**: Right half in landscape mode (departure area)  
- **`UPPER_HALF`**: Upper half in portrait mode (weather area)
- **`LOWER_HALF`**: Lower half in portrait mode (departure area)

#### Semantic Regions (Recommended)
- **`WEATHER_AREA`**: Automatically maps to correct region based on orientation
  - Landscape: `LEFT_HALF`
  - Portrait: `UPPER_HALF`
- **`DEPARTURE_AREA`**: Automatically maps to correct region based on orientation
  - Landscape: `RIGHT_HALF`  
  - Portrait: `LOWER_HALF`
- **`FULL_SCREEN`**: Entire display area

### Usage Examples

```cpp
// Orientation-specific (landscape)
DisplayManager::clearRegion(DisplayRegion::LEFT_HALF);   // Weather area (0-399px)
DisplayManager::clearRegion(DisplayRegion::RIGHT_HALF);  // Departure area (400-799px)

// Orientation-specific (portrait)  
DisplayManager::clearRegion(DisplayRegion::UPPER_HALF);  // Weather area (0-399px height)
DisplayManager::clearRegion(DisplayRegion::LOWER_HALF);  // Departure area (400-799px height)

// Semantic (recommended - adapts automatically)
DisplayManager::clearRegion(DisplayRegion::WEATHER_AREA);   // Weather regardless of orientation
DisplayManager::clearRegion(DisplayRegion::DEPARTURE_AREA); // Departures regardless of orientation
```

**Recommendation**: Use semantic regions (`WEATHER_AREA`, `DEPARTURE_AREA`) instead of orientation-specific regions for code that should work in both orientations.

## API Reference

### DisplayManager Class

#### Initialization
```cpp
// Initialize with default landscape orientation
DisplayManager::init();

// Initialize with specific orientation
DisplayManager::init(DisplayOrientation::PORTRAIT);
```

#### Mode Setting
```cpp
// Set mode and orientation
DisplayManager::setMode(DisplayMode::HALF_AND_HALF, 
                       DisplayOrientation::PORTRAIT);

// Available modes
DisplayMode::HALF_AND_HALF
DisplayMode::WEATHER_ONLY
DisplayMode::DEPARTURES_ONLY

// Available orientations
DisplayOrientation::PORTRAIT
DisplayOrientation::LANDSCAPE
```

#### Display Functions
```cpp
// Half-and-half mode with both data
DisplayManager::displayHalfAndHalf(&weather, &departures);

// Half-and-half mode with weather only
DisplayManager::displayHalfAndHalf(&weather, nullptr);

// Half-and-half mode with departures only
DisplayManager::displayHalfAndHalf(nullptr, &departures);

// Full screen modes
DisplayManager::displayWeatherOnly(weather);
DisplayManager::displayDeparturesOnly(departures);
```

#### Partial Updates
```cpp
// Update individual halves
DisplayManager::updateWeatherHalf(weather);
DisplayManager::updateDepartureHalf(departures);

// Clear specific regions - orientation-specific
DisplayManager::clearRegion(DisplayRegion::LEFT_HALF);   // Landscape left
DisplayManager::clearRegion(DisplayRegion::RIGHT_HALF);  // Landscape right
DisplayManager::clearRegion(DisplayRegion::UPPER_HALF);  // Portrait upper
DisplayManager::clearRegion(DisplayRegion::LOWER_HALF);  // Portrait lower

// Clear semantic regions - adapts to orientation (recommended)
DisplayManager::clearRegion(DisplayRegion::WEATHER_AREA);   // Weather area
DisplayManager::clearRegion(DisplayRegion::DEPARTURE_AREA); // Departure area
DisplayManager::clearRegion(DisplayRegion::FULL_SCREEN);
```

#### Power Management
```cpp
// Put display to sleep
DisplayManager::hibernate();
```

### Data Structures

#### WeatherInfo
```cpp
struct WeatherInfo {
    String temperature;      // "22°C"
    String condition;        // "Partly Cloudy"
    String city;            // "Frankfurt"
    String tempMax;         // "25°C"
    String tempMin;         // "15°C"
    String sunrise;         // "06:30"
    String sunset;          // "20:45"
    WeatherForecast forecast[12];
    int forecastCount;
};
```

#### DepartureData
```cpp
struct DepartureData {
    String stopId;          // "3006907"
    String stopName;        // "Frankfurt Hauptbahnhof"
    std::vector<DepartureInfo> departures;
    int departureCount;
};

struct DepartureInfo {
    String line;            // "S1"
    String direction;       // "Wiesbaden"
    String time;            // "14:23"
    String rtTime;          // "14:25" (real-time)
    String track;           // "3"
    String category;        // "S-Bahn"
};
```

## Performance

### Update Times
- **Full screen update**: ~2.0 seconds
- **Partial update**: ~0.5 seconds
- **Display initialization**: ~1.0 second
- **Hibernation**: ~0.1 seconds

### Memory Usage
- **Display buffer**: ~48KB (800×480 ÷ 8 bits)
- **Font data**: ~15KB (3 fonts)
- **Weather data**: ~2KB
- **Departure data**: ~5KB (typical)
- **Total RAM usage**: ~70KB

### Power Consumption

#### ESP32-C3 Super Mini (Current)
- **Active display update**: ~25mA for 2 seconds
- **Hibernated display**: <10μA
- **ESP32-C3 active**: ~80mA during update
- **ESP32-C3 deep sleep**: <50μA
- **Total cycle**: ~0.111mAh per update

#### XIAO ESP32-C6 (Recommended)
- **Active display update**: ~25mA for 2 seconds (same)
- **Hibernated display**: <10μA (same)
- **ESP32-C6 active**: ~70mA during update (-12% improvement)
- **ESP32-C6 deep sleep**: <35μA (-30% improvement)
- **Total cycle**: ~0.098mAh per update (-12% improvement)

### Battery Life Calculations

Understanding how many display refreshes your battery can provide helps in planning update schedules and battery selection. These calculations include realistic timing and self-discharge effects.

#### Power Consumption Breakdown

**ESP32-C3 Single Display Update Cycle (Realistic):**
```
Component           Current    Duration    Energy Used
─────────────────────────────────────────────────────
ESP32-C3 Active    80mA       7.5s        0.167 mAh
E-Paper Display    25mA       7.5s        0.052 mAh
WiFi Connection    150mA      2.0s        0.083 mAh
API Calls          80mA       1.5s        0.033 mAh
─────────────────────────────────────────────────────
Total per update:                         0.223 mAh
```

**XIAO ESP32-C6 Single Display Update Cycle (Improved):**
```
Component           Current    Duration    Energy Used
─────────────────────────────────────────────────────
ESP32-C6 Active    70mA       7.0s        0.136 mAh
E-Paper Display    25mA       7.0s        0.049 mAh
WiFi Connection    120mA      1.8s        0.060 mAh
API Calls          70mA       1.2s        0.023 mAh
─────────────────────────────────────────────────────
Total per update:                         0.195 mAh (-13%)
```

**Deep Sleep Comparison (between updates):**
```
Microcontroller     Current    Energy/Hour
──────────────────────────────────────────
ESP32-C3 + Display  60μA       0.060 mAh
ESP32-C6 + Display  45μA       0.045 mAh (-25%)
```

**Self-Discharge Effects:**
```
Battery Type        Monthly Loss    Impact on Calculations
─────────────────────────────────────────────────────────
CR123A (Lithium)   2-3%           Minimal (<5% total loss)
Li-ion/LiPo         3-5%           Moderate (5-10% total loss)
Alkaline            7-10%          Significant (10-15% total loss)
```
E-Paper Display    25mA       2.0s        0.014 mAh
WiFi Connection    150mA      1.0s        0.042 mAh
API Calls          80mA       0.5s        0.011 mAh
─────────────────────────────────────────────────────
Total per update:                         0.111 mAh
```

**Deep Sleep (between updates):**
```
Component           Current    Energy/Hour
──────────────────────────────────────────
ESP32-C3 Sleep     50μA       0.050 mAh
E-Paper Hibernated 10μA       0.010 mAh
──────────────────────────────────────────
Total sleep:       60μA       0.060 mAh/hour
```

#### 2500mAh Battery Example (Standard Size)

**Scenario 1: Weather + Departures (5-minute intervals)**

*ESP32-C3 Super Mini (Realistic):*
```
Update frequency: Every 5 minutes (12 updates/hour)
Sleep time: 4 minutes 52.5 seconds between updates

Energy per hour:
- Updates: 12 × 0.223 mAh = 2.68 mAh
- Sleep: 0.060 mAh × 1 hour = 0.060 mAh
- Self-discharge: ~0.035 mAh/hour (3% monthly)
- Total: 2.77 mAh/hour

Battery life: 2500 mAh ÷ 2.77 mAh/hour = 902 hours = 38 days
Realistic range: 35-42 days (accounting for temperature, WiFi signal)
Total refreshes: 902 hours × 12 updates/hour = 10,824 refreshes
```

*XIAO ESP32-C6 (Improved):*
```
Update frequency: Every 5 minutes (12 updates/hour)
Sleep time: 4 minutes 53 seconds between updates

Energy per hour:
- Updates: 12 × 0.195 mAh = 2.34 mAh
- Sleep: 0.045 mAh × 1 hour = 0.045 mAh
- Self-discharge: ~0.035 mAh/hour (3% monthly)
- Total: 2.42 mAh/hour

Battery life: 2500 mAh ÷ 2.42 mAh/hour = 1,033 hours = 43 days (+13%)
Realistic range: 40-47 days (accounting for conditions)
Total refreshes: 1,033 hours × 12 updates/hour = 12,396 refreshes (+13%)
```

**Scenario 2: Weather Only (10-minute intervals)**

*ESP32-C3 Super Mini (Realistic):*
```
Update frequency: Every 10 minutes (6 updates/hour)
Sleep time: 9 minutes 52.5 seconds between updates

Energy per hour:
- Updates: 6 × 0.223 mAh = 1.34 mAh
- Sleep: 0.060 mAh × 1 hour = 0.060 mAh
- Self-discharge: ~0.035 mAh/hour (3% monthly)
- Total: 1.44 mAh/hour

Battery life: 2500 mAh ÷ 1.44 mAh/hour = 1,736 hours = 72 days
Realistic range: 68-78 days (accounting for conditions)
Total refreshes: 1,736 hours × 6 updates/hour = 10,416 refreshes
```

*XIAO ESP32-C6 (Improved):*
```
Update frequency: Every 10 minutes (6 updates/hour)
Sleep time: 9 minutes 53 seconds between updates

Energy per hour:
- Updates: 6 × 0.195 mAh = 1.17 mAh
- Sleep: 0.045 mAh × 1 hour = 0.045 mAh
- Self-discharge: ~0.035 mAh/hour (3% monthly)
- Total: 1.25 mAh/hour

Battery life: 2500 mAh ÷ 1.25 mAh/hour = 2,000 hours = 83 days (+15%)
Realistic range: 78-88 days (accounting for conditions)
Total refreshes: 2,000 hours × 6 updates/hour = 12,000 refreshes (+15%)
```

**Scenario 3: Frequent Updates (2-minute intervals)**

*ESP32-C3 Super Mini (Realistic):*
```
Update frequency: Every 2 minutes (30 updates/hour)
Sleep time: 1 minute 58 seconds between updates

Energy per hour:
Sleep time: 1 minute 52.5 seconds between updates

Energy per hour:
- Updates: 30 × 0.223 mAh = 6.69 mAh
- Sleep: 0.060 mAh × 1 hour = 0.060 mAh
- Self-discharge: ~0.035 mAh/hour (3% monthly)
- Total: 6.79 mAh/hour

Battery life: 2500 mAh ÷ 6.79 mAh/hour = 368 hours = 15 days
Realistic range: 14-17 days (accounting for conditions)
Total refreshes: 368 hours × 30 updates/hour = 11,040 refreshes
```

#### 1000mAh Battery Example (Compact Size)

**Scenario 1: Weather + Departures (5-minute intervals)**

*ESP32-C3 Super Mini (Realistic):*
```
Energy per hour: 2.77 mAh/hour (same calculation as above)

Battery life: 1000 mAh ÷ 2.77 mAh/hour = 361 hours = 15 days
Realistic range: 14-17 days (accounting for conditions)
Total refreshes: 361 hours × 12 updates/hour = 4,332 refreshes
```

*XIAO ESP32-C6 (Improved):*
```
Energy per hour: 2.42 mAh/hour (same calculation as above)

Battery life: 1000 mAh ÷ 2.42 mAh/hour = 413 hours = 17 days (+13%)
Realistic range: 16-19 days (accounting for conditions)
Total refreshes: 413 hours × 12 updates/hour = 4,956 refreshes (+14%)
```

**Scenario 2: Weather Only (10-minute intervals)**

*ESP32-C3 Super Mini (Realistic):*
```
Energy per hour: 1.44 mAh/hour (same calculation as above)

Battery life: 1000 mAh ÷ 1.44 mAh/hour = 694 hours = 29 days
Realistic range: 27-32 days (accounting for conditions)
Total refreshes: 694 hours × 6 updates/hour = 4,164 refreshes
```

*XIAO ESP32-C6 (Improved):*
```
Energy per hour: 1.25 mAh/hour (same calculation as above)

Battery life: 1000 mAh ÷ 1.25 mAh/hour = 800 hours = 33 days (+14%)
Realistic range: 31-36 days (accounting for conditions)
Total refreshes: 800 hours × 6 updates/hour = 4,800 refreshes (+15%)
```

#### Real-World Factors Affecting Battery Life

**Environmental Conditions:**
- **Temperature**: Cold (<10°C) reduces capacity by 10-20%
- **Humidity**: High humidity can affect electronics efficiency
- **WiFi Signal**: Weak signals increase transmission power and duration

**Usage Patterns:**
- **Partial Updates**: Save ~40% energy per refresh
- **Night Mode**: Reduced updates during 11PM-6AM saves 20-30% daily energy
- **Error Handling**: Failed connections consume extra power

**Battery Aging:**
- **Capacity Loss**: Lithium batteries lose 2-3% capacity per year
- **Internal Resistance**: Increases over time, reducing effective capacity
- **Temperature Cycling**: Repeated temperature changes accelerate aging

#### Optimization Strategies

**Smart Scheduling (2500mAh battery example):**
```
Daytime (7AM-11PM, 16 hours): Every 5 minutes = 192 updates
Nighttime (11PM-7AM, 8 hours): Every 30 minutes = 16 updates
Total daily updates: 208 updates

Daily energy consumption:
- Updates: 208 × 0.223 mAh = 46.4 mAh
- Sleep: 0.060 mAh × 24 hours = 1.44 mAh
- Self-discharge: 24 hours × 0.035 mAh = 0.84 mAh
- Total: 48.7 mAh/day

Battery life: 2500 mAh ÷ 48.7 mAh/day = 51 days
Realistic range: 48-55 days (vs 35-42 days without smart scheduling)
Total refreshes: 51 days × 208 updates/day = 10,608 refreshes
```

**Partial Updates Only (saves ~40% power):**
```
Using partial updates instead of full refreshes:
Power per update: 0.223 mAh × 0.6 = 0.134 mAh

5-minute interval example:
- Updates: 12 × 0.134 mAh = 1.61 mAh/hour
- Sleep: 0.060 mAh/hour
- Self-discharge: 0.035 mAh/hour
- Total: 1.70 mAh/hour

Battery life: 2500 mAh ÷ 1.70 mAh/hour = 1,471 hours = 61 days
Realistic range: 58-65 days (vs 35-42 days full refreshes)
Total refreshes: 1,471 hours × 12 updates/hour = 17,652 refreshes
```

#### Summary Table

**ESP32-C3 Super Mini (Current - Realistic Estimates):**

| Battery | Update Interval | Battery Life | Total Refreshes | Refreshes/Day | Notes |
|---------|----------------|--------------|-----------------|---------------|-------|
| 2500mAh | 2 minutes      | 15 days      | 11,040         | 720           | Very frequent |
| 2500mAh | 5 minutes      | 38 days      | 10,824         | 288           | Standard usage |
| 2500mAh | 10 minutes     | 72 days      | 10,416         | 144           | Power optimized |
| 2500mAh | Smart schedule | 51 days      | 10,608         | 208           | Balanced |
| 2500mAh | Partial only   | 61 days      | 17,652         | 288           | Max efficiency |
| 1000mAh | 5 minutes      | 15 days      | 4,332          | 288           | Compact |
| 1000mAh | 10 minutes     | 29 days      | 4,164          | 144           | Compact optimized |

**XIAO ESP32-C6 (Recommended - Better Efficiency):**

| Battery | Update Interval | Battery Life | Total Refreshes | Refreshes/Day | Improvement |
|---------|----------------|--------------|-----------------|---------------|-------------|
| 2500mAh | 2 minutes      | 17 days      | 12,240         | 720           | +13%        |
| 2500mAh | 5 minutes      | 43 days      | 12,396         | 288           | +13%        |
| 2500mAh | 10 minutes     | 83 days      | 11,952         | 144           | +15%        |
| 2500mAh | Smart schedule | 58 days      | 12,064         | 208           | +14%        |
| 2500mAh | Partial only   | 70 days      | 20,160         | 288           | +15%        |
| 1000mAh | 5 minutes      | 17 days      | 4,956          | 288           | +13%        |
| 1000mAh | 10 minutes     | 33 days      | 4,800          | 144           | +14%        |

#### Practical Recommendations

**ESP32-C3 Super Mini (Realistic Expectations):**
- **2500mAh**: 5-10 minute intervals → 1.3-2.4 months battery life
- **1000mAh**: 10-15 minute intervals → 1-1.1 months battery life

**XIAO ESP32-C6 (Recommended - Better Efficiency):**
- **2500mAh**: 5-10 minute intervals → 1.4-2.8 months battery life (+13-15%)
- **1000mAh**: 10-15 minute intervals → 1.1-1.2 months battery life (+13-14%)

**Real-World Performance Factors:**
- **Best case**: Strong WiFi, room temperature, fresh batteries, partial updates
- **Worst case**: Weak WiFi, cold weather, aging batteries, full refreshes
- **Typical**: Mix of conditions, expect values in the middle of ranges

**Why Choose ESP32-C6:**
- **Longer battery life**: 13-15% improvement across all scenarios
- **Better WiFi**: WiFi 6 support, faster connections reduce active time
- **More RAM**: 512KB vs 400KB for complex operations
- **Future-proof**: Dual-core, Zigbee/Thread support
- **Smaller size**: More compact 21×17.5mm footprint

**Update Strategy Guidelines:**
```cpp
// Conservative (long battery life) - both microcontrollers
#define WEATHER_UPDATE_INTERVAL  (15 * 60 * 1000)  // 15 minutes
#define DEPARTURE_UPDATE_INTERVAL (10 * 60 * 1000) // 10 minutes

// Balanced (good responsiveness) - recommended for ESP32-C6
#define WEATHER_UPDATE_INTERVAL  (10 * 60 * 1000)  // 10 minutes  
#define DEPARTURE_UPDATE_INTERVAL (5 * 60 * 1000)  // 5 minutes

// Aggressive (maximum responsiveness) - ESP32-C6 preferred
#define WEATHER_UPDATE_INTERVAL  (5 * 60 * 1000)   // 5 minutes
#define DEPARTURE_UPDATE_INTERVAL (2 * 60 * 1000)  // 2 minutes
```
#define DEPARTURE_UPDATE_INTERVAL (2 * 60 * 1000)  // 2 minutes
```

## Troubleshooting

### Common Issues

#### Display Not Updating
```
Symptoms: Screen remains blank or shows old content
Causes:
- SPI connection issues
- Power supply problems
- Display hibernation state

Solutions:
1. Check all SPI pin connections
2. Verify 3.3V power supply stability
3. Call DisplayManager::init() to wake display
4. Check for error messages in serial output
```

#### Partial Content Missing
```
Symptoms: Only part of screen updates
Causes:
- Incorrect partial window coordinates
- Memory allocation issues
- Font rendering problems

Solutions:
1. Use full screen update: displayHalfAndHalf()
2. Check available RAM: ESP.getFreeHeap()
3. Restart device to clear memory fragmentation
```

#### Text Truncation Issues
```
Symptoms: Text cut off or overlapping
Causes:
- String length calculations incorrect
- Font metrics miscalculated
- Screen boundary detection

Solutions:
1. Use adaptive font sizing
2. Check string length before display
3. Test with different content lengths
```

#### Slow Update Performance
```
Symptoms: Updates take longer than expected
Causes:
- Using full updates instead of partial
- Complex graphics operations
- Memory fragmentation

Solutions:
1. Use partial updates for regular refreshes
2. Implement smart update scheduling
3. Restart device periodically
```

### Debugging Tips

#### Enable Debug Logging
```cpp
// In main.cpp
esp_log_level_set("DISPLAY_MGR", ESP_LOG_DEBUG);
```

#### Monitor Memory Usage
```cpp
ESP_LOGI("DEBUG", "Free heap: %u bytes", ESP.getFreeHeap());
```

#### Check Display Status
```cpp
// Add debugging output in update functions
ESP_LOGI("DISPLAY", "Updating region: %dx%d at (%d,%d)", w, h, x, y);
```

#### Performance Monitoring
```cpp
unsigned long start = millis();
DisplayManager::updateWeatherHalf(weather);
ESP_LOGI("PERF", "Update took %lu ms", millis() - start);
```

## Best Practices

### Update Scheduling
- **Weather**: Update every 10-15 minutes
- **Departures**: Update every 2-5 minutes
- **Full refresh**: Once per hour or on errors
- **Night mode**: Reduce updates during sleep hours

### Error Handling
- Always check data validity before display
- Implement fallback modes for missing data
- Use partial updates to recover from errors
- Provide user feedback for connection issues

### Power Optimization
- Use partial updates when possible
- Hibernate display between updates
- Implement smart scheduling based on time
- Cache data to reduce API calls

### User Experience
- Show loading indicators during updates
- Provide clear error messages
- Use consistent layouts and fonts
- Implement graceful degradation for poor connectivity

---

**📖 [Back to Main README](../README.md)** | **🔧 [Hardware Setup](./hardware-setup.md)** | **⚙️ [Configuration](./configuration.md)**
