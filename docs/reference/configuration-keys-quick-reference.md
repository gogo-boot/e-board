# Configuration Keys Quick Reference

## 🚀 Quick Reference Table

For developers who need to quickly find the right key name for a specific layer:

| Configuration Field | HTML ID | JS/JSON Key | NVS Key | C++ Member |
|-------------------|---------|-------------|---------|------------|
| **Location** |
| City Name | `city-display` | `city` | `city` | `cityName` |
| Latitude | `city-lat` | `cityLat` | `lat` | `latitude` |
| Longitude | `city-lon` | `cityLon` | `lon` | `longitude` |
| **Weather** |
| Update Interval | `weather-interval` | `weatherInterval` | `weatherInt` | `weatherInterval` |
| **Transport** |
| Stop ID | `stop-select` | `stopId` | `stopId` | `selectedStopId` |
| Stop Name | `stop-input` | `stopName` | `stopName` | `selectedStopName` |
| Update Interval | `transport-interval` | `transportInterval` | `transportInt` | `transportInterval` |
| Active Start | `transport-active-start` | `transportActiveStart` | `transStart` | `transportActiveStart` |
| Active End | `transport-active-end` | `transportActiveEnd` | `transEnd` | `transportActiveEnd` |
| Walking Time | `walking-time` | `walkingTime` | `walkTime` | `walkingTime` |
| **Sleep** |
| Sleep Start | `sleep-start` | `sleepStart` | `sleepStart` | `sleepStart` |
| Sleep End | `sleep-end` | `sleepEnd` | `sleepEnd` | `sleepEnd` |
| **Weekend Mode** |
| Weekend Enabled | `weekend-mode` | `weekendMode` | `weekendMode` | `weekendMode` |
| Weekend Transport Start | `weekend-transport-start` | `weekendTransportStart` | `wTransStart` | `weekendTransportStart` |
| Weekend Transport End | `weekend-transport-end` | `weekendTransportEnd` | `wTransEnd` | `weekendTransportEnd` |
| Weekend Sleep Start | `weekend-sleep-start` | `weekendSleepStart` | `wSleepStart` | `weekendSleepStart` |
| Weekend Sleep End | `weekend-sleep-end` | `weekendSleepEnd` | `wSleepEnd` | `weekendSleepEnd` |

## 🔧 Common Operations

### Adding a New Configuration Field

1. **Add to C++ Struct** (`config_struct.h`):
   ```cpp
   int myNewField = 10;  // with default value
   ```

2. **Add NVS Save/Load** (`config_manager.cpp`):
   ```cpp
   // In loadConfig():
   config.myNewField = preferences.getInt("myNewFld", 10);
   
   // In saveConfig():
   preferences.putInt("myNewFld", config.myNewField);
   ```

3. **Add HTML Form** (`config_my_station.html`):
   ```html
   <input type="number" id="my-new-field" value="{{MY_NEW_FIELD}}">
   ```

4. **Add Template Replacement** (`config_page.cpp`):
   ```cpp
   // In handleConfigPage():
   page.replace("{{MY_NEW_FIELD}}", String(g_stationConfig.myNewField));
   
   // In handleSaveConfig():
   if (doc.containsKey("myNewField")) {
       g_stationConfig.myNewField = doc["myNewField"].as<int>();
   }
   ```

5. **Add JavaScript Handling** (`config_my_station.html` script section):
   ```javascript
   var myNewField = document.getElementById('my-new-field').value;
   
   var config = {
       // ... other fields ...
       myNewField: parseInt(myNewField)
   };
   ```

### Key Naming Guidelines

| Layer | Convention | Example | Max Length |
|-------|------------|---------|------------|
| HTML ID | kebab-case | `my-new-field` | No limit |
| JavaScript | camelCase | `myNewField` | No limit |
| JSON Key | camelCase | `myNewField` | No limit |
| NVS Key | abbreviated | `myNewFld` | 15 chars |
| C++ Member | camelCase | `myNewField` | No limit |

### Filter Array Handling

Filters are stored as individual NVS entries:
- `filterCount` → number of filters
- `filter0`, `filter1`, `filter2`, ... → individual filter values

### RTC Memory (Critical Data Only)

Only add to RTC memory if the data is:
- Critical for device operation after deep sleep
- Frequently accessed during wake cycles
- Small in size (RTC memory is limited)

## 📋 Validation Checklist

When adding/modifying configuration fields:

- [ ] NVS key is ≤15 characters
- [ ] Default value is set in C++ struct
- [ ] Load/save implemented in ConfigManager
- [ ] HTML form element added with proper ID
- [ ] Template variable added to config_page.cpp
- [ ] JavaScript variable added to save handler
- [ ] JSON key matches between JS and C++
- [ ] Input validation added if needed
- [ ] Documentation updated

This quick reference should help maintain consistency across all configuration layers while following the established patterns.
