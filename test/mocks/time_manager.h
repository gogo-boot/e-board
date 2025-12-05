#pragma once

#include <ctime>
#include <string>

// Mock TimeManager for native testing
class TimeManager {
public:
    static void setupNTPTime() {
        // No-op for mock
    }

    static std::string getGermanDateTimeString() {
        return "2025-10-29 12:00:00";
    }

    static void printCurrentTime() {
        // No-op for mock
    }

    static bool isTimeSet() {
        return true;
    }

    static bool getCurrentLocalTime(tm& timeinfo) {
        time_t now = time(nullptr);
        localtime_r(&now, &timeinfo);
        return true;
    }

    static bool needsPeriodicSync() {
        return false;
    }

    static void markLastSyncTime() {
        // No-op for mock
    }

    static unsigned long getTimeSinceLastSync() {
        return 0;
    }

    static bool setupNTPTimeWithRetry(int maxRetries = 3) {
        return true;
    }
};

