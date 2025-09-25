#include "config/config_manager.h"
#include <esp_log.h>
#include <vector>

static const char* TAG = "CONFIG_MGR";

// RTC memory allocation - define the static member with defaults
RTC_DATA_ATTR RTCConfigData ConfigManager::rtcConfig = {
    false, // isValid
    0.0, // latitude
    0.0, // longitude
    "", // cityName
    "", // ssid
    "", // ipAddress
    "", // selectedStopId
    "", // selectedStopName
    3, // weatherInterval
    3, // transportInterval
    "06:00", // transportActiveStart
    "09:00", // transportActiveEnd
    5, // walkingTime
    "22:30", // sleepStart
    "05:30", // sleepEnd
    false, // weekendMode
    "08:00", // weekendTransportStart
    "20:00", // weekendTransportEnd
    "23:00", // weekendSleepStart
    "07:00", // weekendSleepEnd
    FILTER_RE | FILTER_S | FILTER_BUS, // filterFlags - Default filters
    true, // configMode
    0 // lastUpdate
};

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::hasValidConfig() {
    if (!rtcConfig.isValid) {
        ESP_LOGI(TAG, "RTC config marked as invalid");
        return false;
    }

    // Validate critical configuration fields
    bool hasValidData = (strlen(rtcConfig.selectedStopId) > 0 &&
        strlen(rtcConfig.ssid) > 0 &&
        rtcConfig.latitude != 0.0 &&
        rtcConfig.longitude != 0.0);

    ESP_LOGI(TAG, "RTC Config validation:");
    ESP_LOGI(TAG, "- SSID: %s", rtcConfig.ssid);
    ESP_LOGI(TAG, "- Stop: %s (%s)", rtcConfig.selectedStopName, rtcConfig.selectedStopId);
    ESP_LOGI(TAG, "- Location: %s (%.6f, %.6f)", rtcConfig.cityName, rtcConfig.latitude, rtcConfig.longitude);
    ESP_LOGI(TAG, "- IP Address: %s", rtcConfig.ipAddress);
    ESP_LOGI(TAG, "- Valid: %s", hasValidData ? "YES" : "NO");

    return hasValidData;
}

void ConfigManager::invalidateConfig() {
    rtcConfig.isValid = false;
    ESP_LOGI(TAG, "RTC config invalidated");
}

// It loads the configuration from NVS into RTC memory.
bool ConfigManager::loadFromNVS() {
    if (!preferences.begin("mystation", true)) {
        // readonly mode
        ESP_LOGE(TAG, "Failed to open NVS for reading");
        return false;
    }

    // Load location data
    rtcConfig.latitude = preferences.getFloat("lat", 0.0);
    rtcConfig.longitude = preferences.getFloat("lon", 0.0);
    String city = preferences.getString("city", "");
    copyString(rtcConfig.cityName, city, sizeof(rtcConfig.cityName));

    // Load network data
    String ssid = preferences.getString("ssid", "");
    copyString(rtcConfig.ssid, ssid, sizeof(rtcConfig.ssid));
    String ip = preferences.getString("ip", "");
    copyString(rtcConfig.ipAddress, ip, sizeof(rtcConfig.ipAddress));

    // Load transport data
    String stopId = preferences.getString("stopId", "");
    copyString(rtcConfig.selectedStopId, stopId, sizeof(rtcConfig.selectedStopId));
    String stopName = preferences.getString("stopName", "");
    copyString(rtcConfig.selectedStopName, stopName, sizeof(rtcConfig.selectedStopName));

    // Load timing configuration
    rtcConfig.weatherInterval = preferences.getInt("weatherInt", 3);
    rtcConfig.transportInterval = preferences.getInt("transportInt", 3);
    rtcConfig.walkingTime = preferences.getInt("walkTime", 5);

    String transStart = preferences.getString("transStart", "06:00");
    copyString(rtcConfig.transportActiveStart, transStart, sizeof(rtcConfig.transportActiveStart));
    String transEnd = preferences.getString("transEnd", "09:00");
    copyString(rtcConfig.transportActiveEnd, transEnd, sizeof(rtcConfig.transportActiveEnd));

    String sleepStart = preferences.getString("sleepStart", "22:30");
    copyString(rtcConfig.sleepStart, sleepStart, sizeof(rtcConfig.sleepStart));
    String sleepEnd = preferences.getString("sleepEnd", "05:30");
    copyString(rtcConfig.sleepEnd, sleepEnd, sizeof(rtcConfig.sleepEnd));

    // Load weekend configuration
    rtcConfig.weekendMode = preferences.getBool("weekendMode", false);
    String wTransStart = preferences.getString("wTransStart", "08:00");
    copyString(rtcConfig.weekendTransportStart, wTransStart, sizeof(rtcConfig.weekendTransportStart));
    String wTransEnd = preferences.getString("wTransEnd", "20:00");
    copyString(rtcConfig.weekendTransportEnd, wTransEnd, sizeof(rtcConfig.weekendTransportEnd));
    String wSleepStart = preferences.getString("wSleepStart", "23:00");
    copyString(rtcConfig.weekendSleepStart, wSleepStart, sizeof(rtcConfig.weekendSleepStart));
    String wSleepEnd = preferences.getString("wSleepEnd", "07:00");
    copyString(rtcConfig.weekendSleepEnd, wSleepEnd, sizeof(rtcConfig.weekendSleepEnd));

    // Load transport filters
    size_t filterCount = preferences.getUInt("filterCount", 3);
    rtcConfig.filterFlags = 0; // Reset flags
    for (size_t i = 0; i < filterCount && i < 8; i++) {
        String key = "filter" + String(i);
        String filter = preferences.getString(key.c_str(), "");
        if (filter == "RE") rtcConfig.filterFlags |= FILTER_RE;
        else if (filter == "R") rtcConfig.filterFlags |= FILTER_R;
        else if (filter == "S-Bahn") rtcConfig.filterFlags |= FILTER_S;
        else if (filter == "Bus") rtcConfig.filterFlags |= FILTER_BUS;
        else if (filter == "U") rtcConfig.filterFlags |= FILTER_U;
        else if (filter == "Tram") rtcConfig.filterFlags |= FILTER_TRAM;
    }

    // Set default filters if none loaded
    if (rtcConfig.filterFlags == 0) {
        rtcConfig.filterFlags = FILTER_RE | FILTER_S | FILTER_BUS;
    }

    // Load system state
    rtcConfig.configMode = preferences.getBool("configMode", true);

    preferences.end();

    // Mark as valid if we have basic data
    bool hasBasicData = (rtcConfig.latitude != 0.0 || rtcConfig.longitude != 0.0 || strlen(rtcConfig.cityName) > 0);
    if (hasBasicData) {
        rtcConfig.isValid = true;
        ESP_LOGI(TAG, "Configuration loaded from NVS to RTC memory");
    } else {
        ESP_LOGI(TAG, "No valid configuration found in NVS");
    }

    return hasBasicData;
}

// Save configuration to NVS. NVS will be used as backup storage. It survives deep sleep and power loss.
bool ConfigManager::saveToNVS() {
    if (!preferences.begin("mystation", false)) {
        // read-write mode
        ESP_LOGE(TAG, "Failed to open NVS for writing");
        return false;
    }

    // Save location data
    preferences.putFloat("lat", rtcConfig.latitude);
    preferences.putFloat("lon", rtcConfig.longitude);
    preferences.putString("city", rtcConfig.cityName);

    // Save network data
    preferences.putString("ssid", rtcConfig.ssid);
    preferences.putString("ip", rtcConfig.ipAddress);

    // Save transport data
    preferences.putString("stopId", rtcConfig.selectedStopId);
    preferences.putString("stopName", rtcConfig.selectedStopName);

    // Save timing configuration
    preferences.putInt("weatherInt", rtcConfig.weatherInterval);
    preferences.putInt("transportInt", rtcConfig.transportInterval);
    preferences.putInt("walkTime", rtcConfig.walkingTime);
    preferences.putString("transStart", rtcConfig.transportActiveStart);
    preferences.putString("transEnd", rtcConfig.transportActiveEnd);
    preferences.putString("sleepStart", rtcConfig.sleepStart);
    preferences.putString("sleepEnd", rtcConfig.sleepEnd);

    // Save weekend configuration
    preferences.putBool("weekendMode", rtcConfig.weekendMode);
    preferences.putString("wTransStart", rtcConfig.weekendTransportStart);
    preferences.putString("wTransEnd", rtcConfig.weekendTransportEnd);
    preferences.putString("wSleepStart", rtcConfig.weekendSleepStart);
    preferences.putString("wSleepEnd", rtcConfig.weekendSleepEnd);

    // Save transport filters
    std::vector<String> filters = getActiveFilters();
    preferences.putUInt("filterCount", filters.size());
    for (size_t i = 0; i < filters.size() && i < 8; i++) {
        String key = "filter" + String(i);
        preferences.putString(key.c_str(), filters[i]);
    }

    // Save system state
    preferences.putBool("configMode", rtcConfig.configMode);

    preferences.end();

    ESP_LOGI(TAG, "Configuration saved from RTC memory to NVS");
    return true;
}

// Helper functions for setting values
void ConfigManager::setLocation(float lat, float lon, const String& city) {
    rtcConfig.latitude = lat;
    rtcConfig.longitude = lon;
    copyString(rtcConfig.cityName, city, sizeof(rtcConfig.cityName));
    rtcConfig.isValid = true;
    ESP_LOGI(TAG, "Location updated: %s (%.6f, %.6f)", city.c_str(), lat, lon);
}

void ConfigManager::setNetwork(const String& ssid, const String& ip) {
    copyString(rtcConfig.ssid, ssid, sizeof(rtcConfig.ssid));
    copyString(rtcConfig.ipAddress, ip, sizeof(rtcConfig.ipAddress));
    ESP_LOGI(TAG, "Network updated: SSID=%s, IP=%s", ssid.c_str(), ip.c_str());
}

void ConfigManager::setStop(const String& stopId, const String& stopName) {
    copyString(rtcConfig.selectedStopId, stopId, sizeof(rtcConfig.selectedStopId));
    copyString(rtcConfig.selectedStopName, stopName, sizeof(rtcConfig.selectedStopName));
    ESP_LOGI(TAG, "Stop updated: %s (%s)", stopName.c_str(), stopId.c_str());
}

void ConfigManager::setTimingConfig(int weatherInt, int transportInt, int walkTime) {
    rtcConfig.weatherInterval = weatherInt;
    rtcConfig.transportInterval = transportInt;
    rtcConfig.walkingTime = walkTime;
    ESP_LOGI(TAG, "Timing updated: Weather=%dh, Transport=%dm, Walk=%dm",
             weatherInt, transportInt, walkTime);
}

void ConfigManager::setActiveHours(const String& start, const String& end) {
    copyString(rtcConfig.transportActiveStart, start, sizeof(rtcConfig.transportActiveStart));
    copyString(rtcConfig.transportActiveEnd, end, sizeof(rtcConfig.transportActiveEnd));
    ESP_LOGI(TAG, "Active hours updated: %s - %s", start.c_str(), end.c_str());
}

void ConfigManager::setSleepHours(const String& start, const String& end) {
    copyString(rtcConfig.sleepStart, start, sizeof(rtcConfig.sleepStart));
    copyString(rtcConfig.sleepEnd, end, sizeof(rtcConfig.sleepEnd));
    ESP_LOGI(TAG, "Sleep hours updated: %s - %s", start.c_str(), end.c_str());
}

void ConfigManager::setWeekendMode(bool enabled) {
    rtcConfig.weekendMode = enabled;
    ESP_LOGI(TAG, "Weekend mode: %s", enabled ? "enabled" : "disabled");
}

void ConfigManager::setWeekendHours(const String& transStart, const String& transEnd,
                                    const String& sleepStart, const String& sleepEnd) {
    copyString(rtcConfig.weekendTransportStart, transStart, sizeof(rtcConfig.weekendTransportStart));
    copyString(rtcConfig.weekendTransportEnd, transEnd, sizeof(rtcConfig.weekendTransportEnd));
    copyString(rtcConfig.weekendSleepStart, sleepStart, sizeof(rtcConfig.weekendSleepStart));
    copyString(rtcConfig.weekendSleepEnd, sleepEnd, sizeof(rtcConfig.weekendSleepEnd));
    ESP_LOGI(TAG, "Weekend hours updated: Transport %s-%s, Sleep %s-%s",
             transStart.c_str(), transEnd.c_str(), sleepStart.c_str(), sleepEnd.c_str());
}

// Filter management
void ConfigManager::setFilterFlag(uint8_t flag, bool enabled) {
    if (enabled) {
        rtcConfig.filterFlags |= flag;
    } else {
        rtcConfig.filterFlags &= ~flag;
    }
}

bool ConfigManager::getFilterFlag(uint8_t flag) {
    return (rtcConfig.filterFlags & flag) != 0;
}

std::vector<String> ConfigManager::getActiveFilters() {
    std::vector<String> filters;
    if (rtcConfig.filterFlags & FILTER_RE) filters.push_back("RE");
    if (rtcConfig.filterFlags & FILTER_R) filters.push_back("R");
    if (rtcConfig.filterFlags & FILTER_S) filters.push_back("S-Bahn");
    if (rtcConfig.filterFlags & FILTER_BUS) filters.push_back("Bus");
    if (rtcConfig.filterFlags & FILTER_U) filters.push_back("U");
    if (rtcConfig.filterFlags & FILTER_TRAM) filters.push_back("Tram");
    return filters;
}

void ConfigManager::setActiveFilters(const std::vector<String>& filters) {
    rtcConfig.filterFlags = 0; // Reset all flags
    for (const String& filter : filters) {
        if (filter == "RE") rtcConfig.filterFlags |= FILTER_RE;
        else if (filter == "R") rtcConfig.filterFlags |= FILTER_R;
        else if (filter == "S-Bahn") rtcConfig.filterFlags |= FILTER_S;
        else if (filter == "Bus") rtcConfig.filterFlags |= FILTER_BUS;
        else if (filter == "U") rtcConfig.filterFlags |= FILTER_U;
        else if (filter == "Tram") rtcConfig.filterFlags |= FILTER_TRAM;
    }
    ESP_LOGI(TAG, "Filters updated: %d active", filters.size());
}

// Internal helper functions
void ConfigManager::copyString(char* dest, const String& src, size_t maxLen) {
    strncpy(dest, src.c_str(), maxLen - 1);
    dest[maxLen - 1] = '\0';
}

void ConfigManager::setDefaults() {
    rtcConfig.isValid = false;
    rtcConfig.latitude = 0.0;
    rtcConfig.longitude = 0.0;
    strcpy(rtcConfig.cityName, "");
    strcpy(rtcConfig.ssid, "");
    strcpy(rtcConfig.ipAddress, "");
    strcpy(rtcConfig.selectedStopId, "");
    strcpy(rtcConfig.selectedStopName, "");
    rtcConfig.weatherInterval = 3;
    rtcConfig.transportInterval = 3;
    strcpy(rtcConfig.transportActiveStart, "06:00");
    strcpy(rtcConfig.transportActiveEnd, "09:00");
    rtcConfig.walkingTime = 5;
    strcpy(rtcConfig.sleepStart, "22:30");
    strcpy(rtcConfig.sleepEnd, "05:30");
    rtcConfig.weekendMode = false;
    strcpy(rtcConfig.weekendTransportStart, "08:00");
    strcpy(rtcConfig.weekendTransportEnd, "20:00");
    strcpy(rtcConfig.weekendSleepStart, "23:00");
    strcpy(rtcConfig.weekendSleepEnd, "07:00");
    rtcConfig.filterFlags = FILTER_RE | FILTER_S | FILTER_BUS;
    rtcConfig.configMode = true;
    rtcConfig.lastUpdate = 0;
}
