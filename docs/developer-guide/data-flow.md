# Configuration Data Store and Loading Process Documentation

## 🏗️ Overview

The ESP32 e-paper board uses a dual-storage strategy for configuration management:
- **NVS (Non-Volatile Storage)**: Complete configuration data (slow but persistent)
- **RTC Memory**: Critical configuration data (fast but limited to 8KB)

This design optimizes for both battery life and persistence across power cycles.

## 📊 Storage Architecture

### NVS Storage (Slow Path)
- **Location**: Flash memory partition
- **Capacity**: Large (limited by flash size)
- **Speed**: Slow (requires flash read/write)
- **Persistence**: Survives power loss, firmware updates, factory resets
- **Purpose**: Complete configuration backup

### RTC Memory (Fast Path)
- **Location**: RTC SRAM
- **Capacity**: Maximum 8KB total
- **Speed**: Fast (direct memory access)
- **Persistence**: Survives deep sleep, lost on power loss
- **Purpose**: Critical data for quick wake-up cycles

## 🔄 Configuration Save Process

### 1. User Configuration Save (Web Interface)
```
User submits form
    ↓
JavaScript collects form data
    ↓
POST /save_config with JSON payload
    ↓
config_page.cpp processes JSON
    ↓
Updates g_stationConfig struct
    ↓
ConfigManager::saveConfig() called
```

### 2. ConfigManager::saveConfig() Implementation
```cpp
bool ConfigManager::saveConfig(const MyStationConfig &config) {
    // Step 1: Save complete config to NVS (slow but persistent)
    if (!preferences.begin("mystation", false)) {
        return false;
    }

    // Save all configuration fields to NVS
    preferences.putFloat("lat", config.latitude);
    preferences.putFloat("lon", config.longitude);
    preferences.putString("city", config.cityName);
    preferences.putString("stopId", config.selectedStopId);
    preferences.putString("stopName", config.selectedStopName);
    preferences.putString("ssid", config.ssid);
    preferences.putString("ip", config.ipAddress);
    preferences.putInt("weatherInt", config.weatherInterval);
    preferences.putInt("transportInt", config.transportInterval);
    // ... and all other fields

    preferences.end();

    // Step 2: Save critical data to RTC memory (fast path)
    saveCriticalToRTC(config);

    return true;
}
```

### 3. Critical Data to RTC Memory
```cpp
void ConfigManager::saveCriticalToRTC(const MyStationConfig &config) {
    // Only save most critical data needed for operation
    ConfigManager::rtcConfig.latitude = config.latitude;
    ConfigManager::rtcConfig.longitude = config.longitude;
    strncpy(ConfigManager::rtcConfig.selectedStopId, config.selectedStopId.c_str(), 63);
    strncpy(ConfigManager::rtcConfig.selectedStopName, config.selectedStopName.c_str(), 127);
    // Note: SSID not saved to RTC (not needed for quick wake)
}
```

## 🔍 Configuration Load Process

### Boot Decision Flow
```
System Boot/Wake
    ↓
hasConfigInNVS() called
    ↓
┌─────────────────────────────────────┐
│ Check if valid config exists        │
│ - Load from NVS                     │
│ - Validate critical fields          │
│   • selectedStopId.length() > 0     │
│   • ssid.length() > 0               │
│   • latitude != 0.0                 │
│   • longitude != 0.0                │
└─────────────────────────────────────┘
    ↓
┌─── Valid Config? ───┐
│                     │
YES                   NO
│                     │
↓                     ↓
runOperationalMode()  runConfigurationMode()
```

### 1. hasConfigInNVS() Implementation
```cpp
bool hasConfigInNVS() {
    ConfigManager& configMgr = ConfigManager::getInstance();

    // Load complete configuration from NVS
    bool configExists = configMgr.loadConfig(g_stationConfig);

    if (!configExists) {
        ESP_LOGI(TAG, "No configuration found in NVS");
        return false;
    }

    // Validate critical configuration fields
    bool hasValidConfig = (g_stationConfig.selectedStopId.length() > 0 &&
                          g_stationConfig.ssid.length() > 0 &&
                          g_stationConfig.latitude != 0.0 &&
                          g_stationConfig.longitude != 0.0);

    return hasValidConfig;
}
```

### 2. Operational Mode Loading Strategy
```cpp
void runOperationalMode() {
    ConfigManager& configMgr = ConfigManager::getInstance();

    // Load complete configuration from NVS first
    if (!configMgr.loadConfig(g_stationConfig)) {
        // Fallback to configuration mode if NVS load fails
        runConfigurationMode();
        return;
    }

    // Continue with normal operation...
}
```

### 3. RTC Memory Fast Path
```cpp
bool ConfigManager::loadCriticalFromRTC(MyStationConfig &config) {

    // Override critical fields with RTC data (faster)
    config.latitude = ConfigManager::rtcConfig.latitude;
    config.longitude = ConfigManager::rtcConfig.longitude;
    config.selectedStopId = String(ConfigManager::rtcConfig.selectedStopId);
    config.selectedStopName = String(ConfigManager::rtcConfig.selectedStopName);

    return true;
}
```

## 🎯 Configuration Mode vs Operational Mode

### Configuration Mode Entry Conditions
- **First boot**: No valid configuration in NVS
- **Invalid config**: Missing critical fields (SSID, stop, location)
- **Manual trigger**: User-initiated configuration reset

### Configuration Mode Behavior
```cpp
void DeviceModeManager::runConfigurationMode() {

    // Setup WiFi AP mode for configuration
    WiFiManager wm;
    MyWiFiManager::setupAPMode(wm);

    // Get location and nearby stops
    getLocationFromGoogle(g_lat, g_lon);
    getCityFromLatLon(g_lat, g_lon);
    getNearbyStops();

    // Start web server
    setupWebServer(server);

    // Stay in loop() to handle web requests
}
```

### Operational Mode Behavior
```cpp
void DeviceModeManager::runOperationalMode() {

    // Load configuration (NVS + RTC fast path)
    configMgr.loadFromNVS();

    // Connect to WiFi in station mode
    MyWiFiManager::setupStationMode();

    // Fetch data and update display
    getDepartureBoard(config.selectedStopId);
    getWeatherFromDWD(g_lat, g_lon, weather);

    // Calculate sleep time and enter deep sleep
    uint64_t sleepTime = calculateSleepTime(config.transportInterval);
    enterDeepSleep(sleepTime);
}
```

## 🔧 Data Flow Diagrams

### Configuration Save Flow
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Web Form      │    │   JavaScript    │    │   JSON POST     │
│   (User Input)  │───▶│   (Collect)     │───▶│   (/save_config)│
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                        │
                                                        ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   RTC Memory    │    │   NVS Storage   │    │  config_page.cpp│
│   (Critical)    │◀───│   (Complete)    │◀───│   (Process)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Configuration Load Flow
```
┌─────────────────┐
│   System Boot   │
└─────────────────┘
          │
          ▼
┌─────────────────┐    ┌─────────────────┐
│ hasConfigInNVS()│───▶│ Load from NVS   │
│  (Decision)     │    │  (Complete)     │
└─────────────────┘    └─────────────────┘
          │                       │
          ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ runConfigMode() │    │runOperational() │
│  (Setup WiFi)   │    │  (Load & Run)   │
└─────────────────┘    └─────────────────┘
                                │
                                ▼
                      ┌─────────────────┐
                      │ loadCriticalRTC()│
                      │  (Fast Path)    │
                      └─────────────────┘
```

## 📋 Key Data Structures

### MyStationConfig (Complete Configuration)
```cpp
struct MyStationConfig {
    // Location
    float latitude = 0.0;
    float longitude = 0.0;
    String cityName;

    // Network
    String ssid;
    String ipAddress;

    // Transport
    String selectedStopId;
    String selectedStopName;
    std::vector<String> oepnvFilters;

    // Timing
    int weatherInterval = 3;
    int transportInterval = 3;
    String transportActiveStart = "06:00";
    String transportActiveEnd = "09:00";
    int walkingTime = 5;
    String sleepStart = "22:30";
    String sleepEnd = "05:30";

    // Weekend mode
    bool weekendMode = false;
    String weekendTransportStart = "08:00";
    String weekendTransportEnd = "20:00";
    String weekendSleepStart = "23:00";
    String weekendSleepEnd = "07:00";
};
```

### RTCConfigData (Critical Data Only)
```cpp
struct RTCConfigData {
    bool isValid;                    // 1 byte
    float latitude;                  // 4 bytes
    float longitude;                 // 4 bytes
    char selectedStopId[64];         // 64 bytes
    char selectedStopName[128];      // 128 bytes
    // Total: ~201 bytes (well under 8KB limit)
};
```

## ⚡ Performance Optimization

### Deep Sleep Wake-up Performance
1. **RTC Check**: `loadCriticalFromRTC()` - ~1ms
2. **NVS Load**: `loadConfig()` - ~50-100ms
3. **Total Boot**: With RTC fast path - ~1-2ms for critical data

### Power Consumption
- **RTC Memory**: ~1µA during deep sleep
- **NVS Access**: ~50mA during read/write operations
- **Strategy**: Use RTC for frequent wake-ups, NVS for persistence

## 🔍 Debugging and Troubleshooting

### Common Issues

#### 1. Empty SSID or Stop ID
```
[MAIN] - SSID:
[MAIN] - Stop: Keine Haltestellen gefunden
```
**Cause**: Configuration not properly saved or loaded
**Fix**: Check NVS write permissions and WiFi connection during config

#### 2. RTC Data Invalid
```
[CONFIG_MGR] Critical config loaded from RTC: Stop=
```
**Cause**: RTC memory cleared or first boot
**Fix**: Normal behavior, will fall back to NVS

#### 3. Configuration Mode Loop
```
[MAIN] Failed to load configuration in operational mode!
[MAIN] Switching to configuration mode...
```
**Cause**: Invalid or missing critical configuration fields
**Fix**: Check NVS data integrity and validation logic

### Debug Logging
```cpp
// Enable debug logging
esp_log_level_set("CONFIG_MGR", ESP_LOG_DEBUG);
esp_log_level_set("MAIN", ESP_LOG_DEBUG);
```

## 🎯 Best Practices

### 1. Configuration Validation
- Always validate critical fields before entering operational mode
- Provide meaningful default values
- Handle corrupted NVS data gracefully

### 2. RTC Memory Usage
- Only store frequently accessed, critical data
- Keep RTC structure small (< 1KB recommended)
- Don't store large strings or arrays

### 3. Error Handling
- Always check NVS operation return values
- Provide fallback mechanisms for data corruption
- Log configuration state changes for debugging

### 4. Power Optimization
- Use RTC fast path for frequent wake-ups
- Minimize NVS access during normal operation
- Save to NVS only when configuration changes

This documentation provides a complete understanding of the configuration data flow, enabling efficient debugging and maintenance of the ESP32 e-paper board system.
