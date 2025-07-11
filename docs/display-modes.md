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
- **Resolution**: 800×480 pixels
- **Colors**: Black and white
- **Refresh**: ~2 seconds full, ~0.5 seconds partial
- **Power**: Ultra-low power consumption

### Pin Connections
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

// Clear specific regions
DisplayManager::clearRegion(DisplayRegion::LEFT_HALF);
DisplayManager::clearRegion(DisplayRegion::RIGHT_HALF);
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
- **Active display update**: ~25mA for 2 seconds
- **Hibernated display**: <10μA
- **ESP32-C3 active**: ~80mA during update
- **ESP32-C3 deep sleep**: <50μA
- **Total cycle**: ~0.05mAh per update

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
