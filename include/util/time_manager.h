#pragma once
#include <Arduino.h>

class TimeManager {
public:
    static void setupNTPTime();
    static String getGermanDateTimeString();
    static void printCurrentTime();
    static bool isTimeSet();
    static bool getCurrentLocalTime(struct tm& timeinfo);

    // Enhanced time management for deep sleep optimization
    static bool needsPeriodicSync();
    static void markLastSyncTime();
    static unsigned long getTimeSinceLastSync();
    static bool setupNTPTimeWithRetry(int maxRetries = 3);

private:
    static const unsigned long SYNC_INTERVAL_MS = 24 * 60 * 60 * 1000UL; // 24 hours
    static const unsigned long MAX_RTC_DRIFT_MS = 1 * 60 * 1000UL; // 1 minutes acceptable drift
};
