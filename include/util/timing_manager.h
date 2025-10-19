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
    // Determine what needs to be updated based on last update times and intervals
    static UpdateType getRequiredUpdates();

    // Get next sleep duration based on mode and next required update
    static uint64_t getNextSleepDuration();

    // Check if currently in active hours (considering weekday/weekend)
    static TimeOfDay getCurrentTimeStatus();

    // Check if transport should be active at current time
    static bool isTransportActiveTime();

    // Check if we're in weekend mode
    static bool isWeekend();

    // Update last refresh timestamps
    static void markWeatherUpdated();
    static void markTransportUpdated();

    // Get time until next required update (in minutes)
    static int getMinutesUntilNextUpdate();

    // Calculate walking time adjusted departure filter (in minutes from now)
    static int getEarliestDepartureTime();
    // Check if it's time for a specific update type
    static bool isTimeForWeatherUpdate();
    static bool isTimeForTransportUpdate();

private:
    // Helper functions
    static int parseTimeString(const String& timeStr); // Convert "HH:MM" to minutes since midnight
    static int getCurrentMinutesSinceMidnight();
    static bool isTimeInRange(int currentMinutes, int startMinutes, int endMinutes);
    static uint32_t getLastWeatherUpdate();
    static uint32_t getLastTransportUpdate();
    static void setLastWeatherUpdate(uint32_t timestamp);
    static void setLastTransportUpdate(uint32_t timestamp);
};
