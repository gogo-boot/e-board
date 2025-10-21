#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <esp_sleep.h>

// Complete RTC memory structure (survives deep sleep, lost on power loss)
struct RTCConfigData {
    // Validation
    bool isValid; // 1 byte

    // Location data
    float latitude; // 4 bytes
    float longitude; // 4 bytes
    char cityName[64]; // 64 bytes

    // Network data
    char ssid[64]; // 64 bytes
    char ipAddress[16]; // 16 bytes

    // Transport data
    char selectedStopId[128]; // 128 bytes
    char selectedStopName[128]; // 128 bytes

    // Timing configuration
    int weatherInterval; // 4 bytes (hours)
    int transportInterval; // 4 bytes (minutes)
    char transportActiveStart[6]; // 6 bytes ("HH:MM")
    char transportActiveEnd[6]; // 6 bytes ("HH:MM")
    int walkingTime; // 4 bytes (minutes)
    char sleepStart[6]; // 6 bytes ("HH:MM")
    char sleepEnd[6]; // 6 bytes ("HH:MM")

    // Weekend configuration
    bool weekendMode; // 1 byte
    char weekendTransportStart[6]; // 6 bytes ("HH:MM")
    char weekendTransportEnd[6]; // 6 bytes ("HH:MM")
    char weekendSleepStart[6]; // 6 bytes ("HH:MM")
    char weekendSleepEnd[6]; // 6 bytes ("HH:MM")

    // Transport filters (simplified - store as bit flags)
    uint8_t filterFlags; // 1 byte (8 different transport types)

    // System state
    bool configMode; // 1 byte
    uint32_t lastUpdate; // 4 bytes (timestamp)

    // Total: ~522 bytes (well under 8KB RTC limit)
};

// Transport filter bit flags
#define FILTER_RE       (1 << 0)   // Regional Express
#define FILTER_R        (1 << 1)   // Regional
#define FILTER_S        (1 << 2)   // S-Bahn
#define FILTER_BUS      (1 << 3)   // Bus
#define FILTER_U        (1 << 4)   // U-Bahn
#define FILTER_TRAM     (1 << 5)   // Tram

class ConfigManager {
public:
    static ConfigManager& getInstance();

    // RTC-based configuration management
    static RTCConfigData& getConfig() { return rtcConfig; }
    static bool hasValidConfig();
    static void invalidateConfig();

    // NVS persistence (backup storage)
    bool loadFromNVS();
    bool saveToNVS();

    // Helper functions for string conversion
    static String getSelectedStopId() { return String(rtcConfig.selectedStopId); }
    static String getSelectedStopName() { return String(rtcConfig.selectedStopName); }
    static String getCityName() { return String(rtcConfig.cityName); }
    static String getSSID() { return String(rtcConfig.ssid); }
    static String getIPAddress() { return String(rtcConfig.ipAddress); }
    static String getTransportActiveStart() { return String(rtcConfig.transportActiveStart); }
    static String getTransportActiveEnd() { return String(rtcConfig.transportActiveEnd); }
    static String getSleepStart() { return String(rtcConfig.sleepStart); }
    static String getSleepEnd() { return String(rtcConfig.sleepEnd); }
    static String getWeekendTransportStart() { return String(rtcConfig.weekendTransportStart); }
    static String getWeekendTransportEnd() { return String(rtcConfig.weekendTransportEnd); }
    static String getWeekendSleepStart() { return String(rtcConfig.weekendSleepStart); }
    static String getWeekendSleepEnd() { return String(rtcConfig.weekendSleepEnd); }

    // Helper functions for setting values
    static void setLocation(float lat, float lon, const String& city);
    static void setNetwork(const String& ssid, const String& ip);
    static void setStop(const String& stopId, const String& stopName);
    static void setTimingConfig(int weatherInt, int transportInt, int walkTime);
    static void setActiveHours(const String& start, const String& end);
    static void setSleepHours(const String& start, const String& end);
    static void setWeekendMode(bool enabled);
    static void setWeekendHours(const String& transStart, const String& transEnd,
                                const String& sleepStart, const String& sleepEnd);

    // Filter management
    static void setFilterFlag(uint8_t flag, bool enabled);
    static bool getFilterFlag(uint8_t flag);
    static std::vector<String> getActiveFilters();
    static void setActiveFilters(const std::vector<String>& filters);

    // Configuration mode management
    static bool isConfigMode() { return rtcConfig.configMode; }
    static void setConfigMode(bool mode) { rtcConfig.configMode = mode; }

    // Check if this is first boot
    static bool isFirstBoot() { return !rtcConfig.isValid; }

    // Public method to set defaults
    static void setDefaults();

    static void printConfiguration(bool fromNVS);

private:
    ConfigManager() = default;
    Preferences preferences;

    static RTCConfigData rtcConfig;

    // Internal helper functions
    static void copyString(char* dest, const String& src, size_t maxLen);
};
