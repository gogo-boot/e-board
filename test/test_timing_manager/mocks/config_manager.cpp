#ifdef NATIVE_TEST

#include "config/config_manager.h"
#include <vector>
#include <cstring>

// Mock RTC memory for native testing
RTCConfigData ConfigManager::rtcConfig = {
    true, // isValid
    DISPLAY_MODE_HALF_AND_HALF, // displayMode
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
    FILTER_RE | FILTER_S | FILTER_BUS, // filterFlags
    false, // configMode
    0 // lastUpdate
};

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::hasValidConfig() {
    return rtcConfig.isValid;
}

void ConfigManager::invalidateConfig() {
    rtcConfig.isValid = false;
}

bool ConfigManager::loadFromNVS() {
    // Mock implementation
    return true;
}

bool ConfigManager::saveToNVS() {
    // Mock implementation
    return true;
}

void ConfigManager::setLocation(float lat, float lon, const String& city) {
    rtcConfig.latitude = lat;
    rtcConfig.longitude = lon;
    copyString(rtcConfig.cityName, city, sizeof(rtcConfig.cityName));
}

void ConfigManager::setNetwork(const String& ssid, const String& ip) {
    copyString(rtcConfig.ssid, ssid, sizeof(rtcConfig.ssid));
    copyString(rtcConfig.ipAddress, ip, sizeof(rtcConfig.ipAddress));
}

void ConfigManager::setStop(const String& stopId, const String& stopName) {
    copyString(rtcConfig.selectedStopId, stopId, sizeof(rtcConfig.selectedStopId));
    copyString(rtcConfig.selectedStopName, stopName, sizeof(rtcConfig.selectedStopName));
}

void ConfigManager::setTimingConfig(int weatherInt, int transportInt, int walkTime) {
    rtcConfig.weatherInterval = weatherInt;
    rtcConfig.transportInterval = transportInt;
    rtcConfig.walkingTime = walkTime;
}

void ConfigManager::setActiveHours(const String& start, const String& end) {
    copyString(rtcConfig.transportActiveStart, start, sizeof(rtcConfig.transportActiveStart));
    copyString(rtcConfig.transportActiveEnd, end, sizeof(rtcConfig.transportActiveEnd));
}

void ConfigManager::setSleepHours(const String& start, const String& end) {
    copyString(rtcConfig.sleepStart, start, sizeof(rtcConfig.sleepStart));
    copyString(rtcConfig.sleepEnd, end, sizeof(rtcConfig.sleepEnd));
}

void ConfigManager::setWeekendMode(bool enabled) {
    rtcConfig.weekendMode = enabled;
}

void ConfigManager::setWeekendHours(const String& transStart, const String& transEnd,
                                    const String& sleepStart, const String& sleepEnd) {
    copyString(rtcConfig.weekendTransportStart, transStart, sizeof(rtcConfig.weekendTransportStart));
    copyString(rtcConfig.weekendTransportEnd, transEnd, sizeof(rtcConfig.weekendTransportEnd));
    copyString(rtcConfig.weekendSleepStart, sleepStart, sizeof(rtcConfig.weekendSleepStart));
    copyString(rtcConfig.weekendSleepEnd, sleepEnd, sizeof(rtcConfig.weekendSleepEnd));
}

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
    rtcConfig.filterFlags = 0;
    for (const String& filter : filters) {
        if (filter == "RE") rtcConfig.filterFlags |= FILTER_RE;
        else if (filter == "R") rtcConfig.filterFlags |= FILTER_R;
        else if (filter == "S-Bahn") rtcConfig.filterFlags |= FILTER_S;
        else if (filter == "Bus") rtcConfig.filterFlags |= FILTER_BUS;
        else if (filter == "U") rtcConfig.filterFlags |= FILTER_U;
        else if (filter == "Tram") rtcConfig.filterFlags |= FILTER_TRAM;
    }
}

void ConfigManager::setDefaults() {
    rtcConfig.isValid = true;
    rtcConfig.displayMode = DISPLAY_MODE_HALF_AND_HALF;
    rtcConfig.weatherInterval = 3;
    rtcConfig.transportInterval = 3;
    rtcConfig.walkingTime = 5;
    std::strcpy(rtcConfig.transportActiveStart, "06:00");
    std::strcpy(rtcConfig.transportActiveEnd, "09:00");
    std::strcpy(rtcConfig.sleepStart, "22:30");
    std::strcpy(rtcConfig.sleepEnd, "05:30");
    rtcConfig.weekendMode = false;
    std::strcpy(rtcConfig.weekendTransportStart, "08:00");
    std::strcpy(rtcConfig.weekendTransportEnd, "20:00");
    std::strcpy(rtcConfig.weekendSleepStart, "23:00");
    std::strcpy(rtcConfig.weekendSleepEnd, "07:00");
    rtcConfig.filterFlags = FILTER_RE | FILTER_S | FILTER_BUS;
    rtcConfig.configMode = false;
}

void ConfigManager::printConfiguration(bool fromNVS) {
    // Mock implementation - just print basic info
    printf("=== Configuration (Mock) ===\n");
    printf("Display Mode: %d\n", rtcConfig.displayMode);
    printf("Weather Interval: %d hours\n", rtcConfig.weatherInterval);
    printf("Transport Interval: %d minutes\n", rtcConfig.transportInterval);
}

void ConfigManager::copyString(char* dest, const String& src, size_t maxLen) {
    size_t len = src.length();
    if (len >= maxLen) {
        len = maxLen - 1;
    }
    std::memcpy(dest, src.c_str(), len);
    dest[len] = '\0';
}

#endif // NATIVE_TEST

