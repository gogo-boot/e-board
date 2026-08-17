#pragma once
#include <Arduino.h>

class TimeManager {
public:
    static String getGermanDateTimeString();
    static void printCurrentTime();
    static bool isTimeSet();
    static bool getCurrentLocalTime(tm& timeinfo);

    // NTP time synchronization (once per day to correct RTC drift)
    static bool needsPeriodicSync();
    static bool setupNTPTimeWithRetry(int maxRetries = 3);

private:
    // Sync once per day — ESP32 RTC drifts ~5-15 seconds/day (150 ppm)
    static constexpr uint32_t SYNC_INTERVAL_SECONDS = 24 * 60 * 60; // 24 hours
    static constexpr int NTP_TIMEOUT_MS = 5000; // Max wait for NTP response
};
