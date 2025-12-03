#pragma once

#include <Arduino.h>
#include "config/config_manager.h"

enum class UpdateType {
    WEATHER_ONLY,
    TRANSPORT_ONLY,
    BOTH
};

enum class TimeOfDay {
    ACTIVE_HOURS,
    SLEEP_HOURS,
    TRANSITION
};

class TimingManager {
public:
    // Get next sleep duration based on mode and next required update
    static uint64_t getNextSleepDurationSeconds();

    // Check if transport should be active at current time
    static bool isTransportActiveTime();

    // Check if we're in weekend mode
    static bool isWeekend();

    // Update last refresh timestamps
    static void markWeatherUpdated();
    static void markTransportUpdated();

    // Check if it's time for a specific update type
    static bool isTimeForWeatherUpdate();
    static bool isTimeForTransportUpdate();

    // Get effective display mode (considers temporary mode)
    static uint8_t getEffectiveDisplayMode();

    // RTC timestamp management (public for testing)
    static uint32_t getLastWeatherUpdate();
    static uint32_t getLastTransportUpdate();
    static void setLastWeatherUpdate(uint32_t timestamp);
    static void setLastTransportUpdate(uint32_t timestamp);
    // OTA update timestamp management (public for testing)
    static uint32_t getLastOTACheck();
    static void setLastOTACheck(uint32_t timestamp);

private:
    // Helper functions
    static int parseTimeString(const String& timeStr); // Convert "HH:MM" to minutes since midnight
    static int getCurrentMinutesSinceMidnight();
    static bool isTimeInRange(int currentMinutes, int startMinutes, int endMinutes);
    static uint32_t calculateNextOTACheckTime(uint32_t currentTimeSeconds);
    // Transport active hours helpers
    static bool isTransportActiveAtTime(uint32_t timestamp);
    static uint32_t calculateNextActiveTransportTime(uint32_t currentTime);

    // Sleep duration calculation helpers
    static uint32_t calculateNextWeatherUpdate(uint32_t currentTimeSeconds);
    static uint32_t calculateNextTransportUpdate(uint32_t currentTimeSeconds);
    static uint32_t findNearestUpdateTime(uint32_t weather, uint32_t transport, uint32_t ota);
    static uint32_t adjustForTransportActiveHours(uint32_t nearestUpdate, uint32_t nextTransport, uint32_t nextWeather,
                                                  uint32_t nextOTA,
                                                  uint32_t currentTime, bool& isOTAUpdate);
    static uint32_t adjustForSleepPeriod(uint32_t nearestUpdate, bool isOTAUpdate);
    static bool isWeekend(time_t timestamp);
    static bool isInDeepSleepPeriod();
    static uint16_t getCurrentMin();
    static uint16_t getSleepStartMin();
    static uint16_t getSleepEndMin();
};
