#include "config_manager.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char* TAG = "CONFIG_MGR";

// RTC memory allocation - define the static member
RTC_DATA_ATTR RTCConfigData ConfigManager::rtcConfig = {false, 0.0, 0.0, "", ""};

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(MyStationConfig &config) {
    if (!preferences.begin("mystation", true)) { // readonly mode
        ESP_LOGE(TAG, "Failed to open NVS");
        return false;
    }
    
    // Load from NVS (slower but complete data)
    config.latitude = preferences.getFloat("lat", 0.0);
    config.longitude = preferences.getFloat("lon", 0.0);
    config.cityName = preferences.getString("city", "");
    config.selectedStopId = preferences.getString("stopId", "");
    config.selectedStopName = preferences.getString("stopName", "");
    config.ssid = preferences.getString("ssid", "");
    config.ipAddress = preferences.getString("ip", "");
    
    // Load new configuration values with defaults
    config.weatherInterval = preferences.getInt("weatherInt", 3);
    config.transportInterval = preferences.getInt("transportInt", 3);
    config.transportActiveStart = preferences.getString("transportStart", "06:00");
    config.transportActiveEnd = preferences.getString("transportEnd", "09:00");
    config.walkingTime = preferences.getInt("walkTime", 5);
    config.sleepStart = preferences.getString("sleepStart", "22:30");
    config.sleepEnd = preferences.getString("sleepEnd", "05:30");
    config.weekendMode = preferences.getBool("weekendMode", false);
    config.weekendTransportStart = preferences.getString("wTransportStart", "08:00");
    config.weekendTransportEnd = preferences.getString("wTransportEnd", "20:00");
    config.weekendSleepStart = preferences.getString("wSleepStart", "23:00");
    config.weekendSleepEnd = preferences.getString("wSleepEnd", "07:00");
    
    // Load ÖPNV filters
    size_t filterCount = preferences.getUInt("filterCount", 3);
    config.oepnvFilters.clear();
    for (size_t i = 0; i < filterCount && i < 10; i++) { // Max 10 filters
        String key = "filter" + String(i);
        String filter = preferences.getString(key.c_str(), "");
        if (filter.length() > 0) {
            config.oepnvFilters.push_back(filter);
        }
    }
    
    // Set defaults if empty
    if (config.oepnvFilters.empty()) {
        config.oepnvFilters = {"RE", "S-Bahn", "Bus"};
    }
    
    preferences.end();
    
    ESP_LOGI(TAG, "Config loaded from NVS: City=%s, Stop=%s, Filters=%d", 
             config.cityName.c_str(), config.selectedStopName.c_str(), config.oepnvFilters.size());
    return (config.latitude != 0.0 || config.longitude != 0.0 || config.cityName.length() > 0);
}

bool ConfigManager::saveConfig(const MyStationConfig &config) {
    if (!preferences.begin("mystation", false)) { // read-write mode
        ESP_LOGE(TAG, "Failed to open NVS for writing");
        return false;
    }
    
    // Save to NVS
    preferences.putFloat("lat", config.latitude);
    preferences.putFloat("lon", config.longitude);
    preferences.putString("city", config.cityName);
    preferences.putString("stopId", config.selectedStopId);
    preferences.putString("stopName", config.selectedStopName);
    preferences.putString("ssid", config.ssid);
    preferences.putString("ip", config.ipAddress);
    
    // Save new configuration values
    preferences.putInt("weatherInt", config.weatherInterval);
    preferences.putInt("transportInt", config.transportInterval);
    preferences.putString("transportStart", config.transportActiveStart);
    preferences.putString("transportEnd", config.transportActiveEnd);
    preferences.putInt("walkTime", config.walkingTime);
    preferences.putString("sleepStart", config.sleepStart);
    preferences.putString("sleepEnd", config.sleepEnd);
    preferences.putBool("weekendMode", config.weekendMode);
    preferences.putString("wTransportStart", config.weekendTransportStart);
    preferences.putString("wTransportEnd", config.weekendTransportEnd);
    preferences.putString("wSleepStart", config.weekendSleepStart);
    preferences.putString("wSleepEnd", config.weekendSleepEnd);
    
    // Save ÖPNV filters
    preferences.putUInt("filterCount", config.oepnvFilters.size());
    for (size_t i = 0; i < config.oepnvFilters.size() && i < 10; i++) {
        String key = "filter" + String(i);
        preferences.putString(key.c_str(), config.oepnvFilters[i]);
    }
    
    preferences.end();
    
    // Also save critical data to RTC memory for fast wake
    saveCriticalToRTC(config);
    
    ESP_LOGI(TAG, "Config saved to NVS and RTC");
    return true;
}

void ConfigManager::saveCriticalToRTC(const MyStationConfig &config) {
    ConfigManager::rtcConfig.isValid = true;
    ConfigManager::rtcConfig.latitude = config.latitude;
    ConfigManager::rtcConfig.longitude = config.longitude;
    strncpy(ConfigManager::rtcConfig.selectedStopId, config.selectedStopId.c_str(), sizeof(ConfigManager::rtcConfig.selectedStopId) - 1);
    strncpy(ConfigManager::rtcConfig.selectedStopName, config.selectedStopName.c_str(), sizeof(ConfigManager::rtcConfig.selectedStopName) - 1);
    ConfigManager::rtcConfig.selectedStopId[sizeof(ConfigManager::rtcConfig.selectedStopId) - 1] = '\0';
    ConfigManager::rtcConfig.selectedStopName[sizeof(ConfigManager::rtcConfig.selectedStopName) - 1] = '\0';
}

bool ConfigManager::loadCriticalFromRTC(MyStationConfig &config) {
    if (!ConfigManager::rtcConfig.isValid) {
        return false;
    }
    
    config.latitude = ConfigManager::rtcConfig.latitude;
    config.longitude = ConfigManager::rtcConfig.longitude;
    config.selectedStopId = String(ConfigManager::rtcConfig.selectedStopId);
    config.selectedStopName = String(ConfigManager::rtcConfig.selectedStopName);
    
    ESP_LOGI(TAG, "Critical config loaded from RTC: Stop=%s", config.selectedStopName.c_str());
    return true;
}

bool ConfigManager::isFirstBoot() {
    return !ConfigManager::rtcConfig.isValid;
}
