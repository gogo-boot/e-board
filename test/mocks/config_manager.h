#pragma once
#include "esp32_mocks.h"
#include <cstdint>

// Mock RTC configuration data structure
struct RTCConfigData {
    bool isValid = true;
    uint8_t displayMode = 0; // 0=half_and_half, 1=weather_only, 2=departure_only
    float latitude = 50.0f;
    float longitude = 8.0f;
    char cityName[64] = "Frankfurt";
    char ssid[64] = "TestSSID";
    char ipAddress[16] = "192.168.1.100";
    char selectedStopId[128] = "test_stop_id";
    char selectedStopName[128] = "Test Stop";
    int weatherInterval = 2; // hours
    int transportInterval = 15; // minutes
    char transportActiveStart[6] = "06:00";
    char transportActiveEnd[6] = "22:00";
    int walkingTime = 5; // minutes
    char sleepStart[6] = "23:00";
    char sleepEnd[6] = "05:30";
    bool weekendMode = true;
    char weekendTransportStart[6] = "08:00";
    char weekendTransportEnd[6] = "20:00";
    char weekendSleepStart[6] = "00:00";
    char weekendSleepEnd[6] = "07:00";
    uint8_t filterFlags = 0;
    bool configMode = false;
    uint32_t lastUpdate = 0;
};

class ConfigManager {
public:
    static RTCConfigData& getConfig() {
        static RTCConfigData config;
        return config;
    }

    static String getTransportActiveStart() {
        return String(getConfig().transportActiveStart);
    }

    static String getTransportActiveEnd() {
        return String(getConfig().transportActiveEnd);
    }

    static String getWeekendTransportStart() {
        return String(getConfig().weekendTransportStart);
    }

    static String getWeekendTransportEnd() {
        return String(getConfig().weekendTransportEnd);
    }

    static String getSleepStart() {
        return String(getConfig().sleepStart);
    }

    static String getSleepEnd() {
        return String(getConfig().sleepEnd);
    }

    static String getWeekendSleepStart() {
        return String(getConfig().weekendSleepStart);
    }

    static String getWeekendSleepEnd() {
        return String(getConfig().weekendSleepEnd);
    }
};
