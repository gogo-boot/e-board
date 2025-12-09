#ifdef NATIVE_TEST

#include "config/config_manager.h"
#include <vector>
#include <cstring>

// Mock RTC memory for native testing
RTCConfigData ConfigManager::rtcConfig = {
    false, // wifiConfigured
    DISPLAY_MODE_HALF_AND_HALF, // displayMode
    0.0f, // latitude
    0.0f, // longitude
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
    true, // otaEnabled
    "03:00", // otaCheckTime
    (uint16_t)(
        FILTER_R | FILTER_S | FILTER_U | FILTER_TRAM | FILTER_BUS | FILTER_HIGHFLOOR | FILTER_FERRY | FILTER_CALLBUS),
    // filterFlags
    false, // configMode
    0, // lastUpdate
    false, // inTemporaryMode
    0xFF, // temporaryDisplayMode
    0 // temporaryModeActivationTime
};

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
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

void ConfigManager::setFilterFlag(uint16_t flag, bool enabled) {
    if (enabled) {
        rtcConfig.filterFlags |= flag;
    } else {
        rtcConfig.filterFlags &= ~flag;
    }
}

bool ConfigManager::getFilterFlag(uint16_t flag) {
    return (rtcConfig.filterFlags & flag) != 0;
}

std::vector<String> ConfigManager::getActiveFilters() {
    std::vector<String> filters;
    if (rtcConfig.filterFlags & FILTER_R) filters.push_back("R");
    if (rtcConfig.filterFlags & FILTER_S) filters.push_back("S-Bahn");
    if (rtcConfig.filterFlags & FILTER_U) filters.push_back("U");
    if (rtcConfig.filterFlags & FILTER_TRAM) filters.push_back("Tram");
    // Check if all bus types are enabled
    uint16_t allBusFlags = FILTER_BUS | FILTER_CALLBUS | FILTER_HIGHFLOOR;
    if ((rtcConfig.filterFlags & allBusFlags) == allBusFlags) {
        filters.push_back("Bus");
    }
    if (rtcConfig.filterFlags & FILTER_FERRY) filters.push_back("Fähre");
    return filters;
}

void ConfigManager::setActiveFilters(const std::vector<String>& filters) {
    rtcConfig.filterFlags = 0;
    for (const String& filter : filters) {
        // Support both short and long filter names
        if (filter == "R" || filter == "RE" || filter == "Regional") {
            rtcConfig.filterFlags |= FILTER_R;
        } else if (filter == "S" || filter == "S-Bahn") {
            rtcConfig.filterFlags |= FILTER_S;
        } else if (filter == "U" || filter == "U-Bahn") {
            rtcConfig.filterFlags |= FILTER_U;
        } else if (filter == "Tram" || filter == "Straßenbahn") {
            rtcConfig.filterFlags |= FILTER_TRAM;
        } else if (filter == "Bus") {
            rtcConfig.filterFlags |= FILTER_BUS | FILTER_CALLBUS | FILTER_HIGHFLOOR;
        } else if (filter == "Fähre" || filter == "Ferry") {
            rtcConfig.filterFlags |= FILTER_FERRY;
        }
    }
}

void ConfigManager::setDefaults() {
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
    rtcConfig.filterFlags = FILTER_S | FILTER_BUS;
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

