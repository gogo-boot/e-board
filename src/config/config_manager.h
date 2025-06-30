#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "esp_sleep.h"
#include "config_struct.h"

// RTC memory structure for critical data (survives deep sleep)
struct RTCConfigData {
    bool isValid;
    float latitude;
    float longitude;
    char selectedStopId[64];
    char selectedStopName[128];
};

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Load configuration from NVS
    bool loadConfig(MyStationConfig &config);
    
    // Save configuration to NVS
    bool saveConfig(const MyStationConfig &config);
    
    // Save only critical data to RTC memory (for deep sleep)
    void saveCriticalToRTC(const MyStationConfig &config);
    
    // Load critical data from RTC memory (after deep sleep wake)
    bool loadCriticalFromRTC(MyStationConfig &config);
    
    // Check if this is first boot or wake from deep sleep
    bool isFirstBoot();
    
private:
    ConfigManager() = default;
    Preferences preferences;
    
    static RTCConfigData rtcConfig;
};
