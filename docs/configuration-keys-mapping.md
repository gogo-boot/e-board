# Configuration Keys and IDs Mapping Documentation

This document provides a comprehensive mapping of all configuration keys, IDs, and variable names used across the different layers of the ESP32 e-paper board project.

## 🏗️ Architecture Overview

The configuration system uses a 4-layer architecture:

1. **HTML Layer**: User interface elements with descriptive IDs
2. **JavaScript Layer**: Variables and JSON keys for API communication  
3. **NVS (Preferences) Layer**: Efficient storage keys ≤15 characters
4. **RTC Memory Layer**: Critical data that survives deep sleep

## 📋 Complete Configuration Mapping

### Location/Weather Configuration

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| City Name | `city-display`, `city-input` | `city` | `city` | `city` | - | String | `""` |
| Latitude | `city-lat`, `lat-display` | `cityLat` | `cityLat` | `lat` | `latitude` | Float | `0.0` |
| Longitude | `city-lon`, `lon-display` | `cityLon` | `cityLon` | `lon` | `longitude` | Float | `0.0` |
| Weather Update Interval | `weather-interval` | `weatherInterval` | `weatherInterval` | `weatherInt` | - | Int | `3` |

### Transport/ÖPNV Configuration

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| Selected Stop ID | `stop-select` | `stopId` | `stopId` | `stopId` | `selectedStopId` | String | `""` |
| Selected Stop Name | `stop-input` | `stopName` | `stopName` | `stopName` | `selectedStopName` | String | `""` |
| Transport Update Interval | `transport-interval` | `transportInterval` | `transportInterval` | `transportInt` | - | Int | `3` |
| Transport Active Start | `transport-active-start` | `transportActiveStart` | `transportActiveStart` | `transStart` | - | String | `"06:00"` |
| Transport Active End | `transport-active-end` | `transportActiveEnd` | `transportActiveEnd` | `transEnd` | - | String | `"09:00"` |
| Walking Time | `walking-time` | `walkingTime` | `walkingTime` | `walkTime` | - | Int | `5` |
| Transport Filters | `filter-chips` (`.chip[data-type]`) | `filters` | `filters` | `filter0`, `filter1`, ... | - | Array | `["RE", "S-Bahn", "Bus"]` |

### Sleep/Power Management Configuration

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| Sleep Start Time | `sleep-start` | `sleepStart` | `sleepStart` | `sleepStart` | - | String | `"22:30"` |
| Sleep End Time | `sleep-end` | `sleepEnd` | `sleepEnd` | `sleepEnd` | - | String | `"05:30"` |
| Weekend Mode Enabled | `weekend-mode` | `weekendMode` | `weekendMode` | `weekendMode` | - | Bool | `false` |

### Weekend Configuration

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| Weekend Transport Start | `weekend-transport-start` | `weekendTransportStart` | `weekendTransportStart` | `wTransStart` | - | String | `"08:00"` |
| Weekend Transport End | `weekend-transport-end` | `weekendTransportEnd` | `weekendTransportEnd` | `wTransEnd` | - | String | `"20:00"` |
| Weekend Sleep Start | `weekend-sleep-start` | `weekendSleepStart` | `weekendSleepStart` | `wSleepStart` | - | String | `"23:00"` |
| Weekend Sleep End | `weekend-sleep-end` | `weekendSleepEnd` | `weekendSleepEnd` | `wSleepEnd` | - | String | `"07:00"` |

### Network Information (Read-Only)

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| WiFi SSID | `router` | - | - | `ssid` | - | String | `""` |
| IP Address | `ip` | - | - | `ip` | - | String | `""` |
| mDNS Address | `mdns` | - | - | - | - | String | `""` |

### System Configuration

| Field | HTML ID | JavaScript Variable | JSON Key | NVS Key | RTC Memory | Type | Default |
|-------|---------|-------------------|----------|---------|------------|------|---------|
| Configuration Mode | - | - | - | `configMode` | - | Bool | `true` |
| Filter Count | - | - | - | `filterCount` | - | UInt | `3` |
| RTC Data Valid Flag | - | - | - | - | `isValid` | Bool | `false` |

## 🔄 Data Flow Architecture

### Configuration Save Flow
```
HTML Form Elements 
    ↓ (JavaScript collects values)
JavaScript Variables 
    ↓ (POST /save_config)
JSON Payload 
    ↓ (config_page.cpp processes)
C++ Config Struct 
    ↓ (config_manager.cpp)
NVS Storage + RTC Memory
```

### Configuration Load Flow
```
NVS Storage 
    ↓ (config_manager.cpp)
C++ Config Struct 
    ↓ (config_page.cpp template replacement)
HTML Template Variables 
    ↓ (Server renders page)
HTML Form Elements
```

## 📝 Key Naming Conventions

### HTML IDs
- Use kebab-case: `transport-active-start`
- Descriptive and human-readable
- Include element type hint: `-input`, `-select`, `-display`

### JavaScript Variables
- Use camelCase: `transportActiveStart`
- Match JSON key names for API consistency
- Clear, descriptive naming

### JSON Keys (API)
- Use camelCase: `transportActiveStart`
- Full, descriptive names for API clarity
- Match C++ struct member names

### NVS Keys
- Maximum 15 characters (ESP32 limitation)
- Use abbreviated but recognizable names
- Examples: `transStart`, `weatherInt`, `wTransStart`

### RTC Memory Fields
- Use camelCase: `selectedStopId`
- Only critical data needed after deep sleep wake
- Fixed-size char arrays for strings

## 🗂️ File Locations

| Layer | File Path | Purpose |
|-------|-----------|---------|
| HTML | `/data/config_my_station.html` | User interface template |
| JavaScript | `/data/config_my_station.html` (embedded) | Form handling and API calls |
| JSON Processing | `/src/config/config_page.cpp` | HTTP request handling |
| Config Struct | `/src/config/config_struct.h` | Data structure definition |
| NVS Management | `/src/config/config_manager.cpp` | Persistent storage |
| RTC Structure | `/src/config/config_manager.h` | Deep sleep data structure |

## 🔧 Implementation Notes

### NVS Key Length Limitation
ESP32 NVS keys are limited to 15 characters. All keys have been carefully chosen to stay within this limit while remaining recognizable.

### RTC Memory Usage
Only critical data (location and selected stop) is stored in RTC memory to minimize power consumption during deep sleep.

### Template Variables
HTML template uses `{{VARIABLE_NAME}}` syntax for server-side replacement. These are processed by `config_page.cpp`.

### Filter Storage
Transport filters are stored as individual NVS keys (`filter0`, `filter1`, etc.) with a count (`filterCount`) to handle variable-length arrays.

## 🔍 Debugging

### Logging Tags
- `CONFIG_MGR`: ConfigManager operations
- `CONFIG`: config_page.cpp operations

### Common Issues
1. **NVS Key Too Long**: Ensure all keys ≤15 characters
2. **Template Not Replaced**: Check `config_page.cpp` template replacement logic
3. **RTC Data Lost**: Check if deep sleep is properly configured
4. **JSON Parse Error**: Verify JavaScript JSON structure matches expected format

## 📊 Key Statistics

- **Total Configuration Fields**: 20
- **NVS Keys Used**: 22 (including filter array and metadata)
- **RTC Memory Fields**: 5 (critical data only)
- **HTML Form Elements**: 18
- **Template Variables**: 16

This mapping ensures consistent configuration management across all layers while optimizing for ESP32 constraints and user experience.
