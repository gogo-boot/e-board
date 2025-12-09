#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <vector>

// Complete RTC memory structure (survives deep sleep, lost on power loss)
struct RTCConfigData {
    // Validation
    bool wifiConfigured; // 1 byte - true when WiFi credentials are validated AND internet is accessible

    // Display configuration
    uint8_t displayMode; // 1 byte (0=half_and_half, 1=weather_only, 2=departure_only)

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

    // OTA configuration
    bool otaEnabled; // 1 byte
    char otaCheckTime[6]; // 6 bytes ("HH:MM")

    // Transport filters (simplified - store as bit flags)
    uint16_t filterFlags; // 2 byte

    // System state
    bool configMode; // 1 byte
    uint32_t lastUpdate; // 4 bytes (timestamp)

    // Temporary button mode (ESP32-S3 only)
    // NOTE: These are volatile to prevent compiler optimization that might prevent
    // RTC memory writes from completing before deep sleep
    volatile bool inTemporaryMode; // 1 byte - flag if we're in temporary mode
    volatile uint8_t temporaryDisplayMode; // 1 byte - temporary override mode (0xFF = none)
    volatile uint32_t temporaryModeActivationTime; // 4 bytes - when temporary mode was activated (epoch time)

    // Total: ~535 bytes (well under 8KB RTC limit)
};

/*
 * RMV API Product Values (from official RMV HAFAS API documentation)
 * These bit values must match exactly what RMV expects in the products parameter
 *
 * Regionalverkehrszug (Regional Express/Regional) = 4
 * S-Bahn                                          = 8
 * U-Bahn                                          = 16
 * Straßenbahn (Tram)                             = 32
 * Bus                                             = 64
 * Hochflurbus (High-floor Bus)                   = 128
 * Fähre/Schiff (Ferry/Ship)                      = 256
 * Ast/Rufbus (Call Bus/On-demand)                = 512
 */
// Transport filter bit flags - MUST match RMV API product values!
#define FILTER_R         4      // Regionalverkehrszug (Regional trains)
#define FILTER_S         8      // S-Bahn
#define FILTER_U         16     // U-Bahn
#define FILTER_TRAM      32     // Straßenbahn (Tram)
#define FILTER_BUS       64     // Bus
#define FILTER_HIGHFLOOR 128    // Hochflurbus (High-floor Bus)
#define FILTER_FERRY     256    // Fähre/Schiff (Ferry/Ship)
#define FILTER_CALLBUS   512    // Ast/Rufbus (Call Bus/On-demand)

// Maximum number of transport filters
constexpr size_t MAX_TRANSPORT_FILTERS = 8;

// Display mode constants
#define DISPLAY_MODE_HALF_AND_HALF  0
#define DISPLAY_MODE_WEATHER_ONLY   1
#define DISPLAY_MODE_TRANSPORT_ONLY 2

class ConfigManager {
public:
    static ConfigManager& getInstance();

    // RTC-based configuration management
    static RTCConfigData& getConfig() { return rtcConfig; }
    static bool hasValidConfig();
    static void invalidateConfig();

    // NVS persistence (backup storage)
    bool loadFromNVS(bool force);
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

    // Display mode configuration
    static uint8_t getDisplayMode() { return rtcConfig.displayMode; }
    static void setDisplayMode(uint8_t mode) { rtcConfig.displayMode = mode; }

    // Filter management
    static void setFilterFlag(uint16_t flag, bool enabled);
    static bool getFilterFlag(uint16_t flag);
    static std::vector<String> getActiveFilters();
    static void setActiveFilters(const std::vector<String>& filters);

    // Extract stop name from stopId format: "@O=StopName@"
    static String getStopNameFromId();

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
