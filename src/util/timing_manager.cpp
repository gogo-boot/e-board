#include "util/timing_manager.h"
#ifdef NATIVE_TEST
#include "time_manager.h"
#include "esp_log.h"
#include "mock_time.h"
#define GET_CURRENT_TIME() MockTime::now()
#else
#include "util/time_manager.h"
#include <esp_log.h>
#define GET_CURRENT_TIME() ({ time_t t; time(&t); t; })
#endif
#include <time.h>

static const char* TAG = "TIMING_MGR";

// RTC memory for storing last update timestamps
RTC_DATA_ATTR uint32_t lastWeatherUpdate = 0;
RTC_DATA_ATTR uint32_t lastTransportUpdate = 0;
RTC_DATA_ATTR uint32_t lastOTACheck = 0;

// ============================================================================
// Helper Functions for Sleep Duration Calculation
// ============================================================================

bool TimingManager::isWeekendTime(time_t timestamp) {
    RTCConfigData& config = ConfigManager::getConfig();
    if (!config.weekendMode) {
        return false;
    }
    struct tm timeInfo;
    localtime_r(&timestamp, &timeInfo);
    return (timeInfo.tm_wday == 0 || timeInfo.tm_wday == 6);
}

uint32_t TimingManager::calculateNextWeatherUpdate(uint32_t currentTimeSeconds, uint8_t displayMode) {
    // Weather updates needed for weather_only (1) and half_and_half (0) modes
    if (displayMode != 0 && displayMode != 1) {
        return 0;
    }

    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();
    uint32_t intervalSeconds = config.weatherInterval * 3600; // hours to seconds

    uint32_t nextUpdate = (lastUpdate == 0) ? currentTimeSeconds : lastUpdate + intervalSeconds;

    ESP_LOGI(TAG, "Weather interval: %u hours (%u seconds), Next weather update: %u",
             config.weatherInterval, intervalSeconds, nextUpdate);

    return nextUpdate;
}

uint32_t TimingManager::calculateNextTransportUpdate(uint32_t currentTimeSeconds, uint8_t displayMode) {
    // Departure updates needed for departure_only (2) and half_and_half (0) modes
    if (displayMode != 0 && displayMode != 2) {
        return 0;
    }

    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastTransportUpdate();
    uint32_t intervalSeconds = config.transportInterval * 60; // minutes to seconds

    uint32_t nextUpdate = (lastUpdate == 0) ? currentTimeSeconds : lastUpdate + intervalSeconds;

    ESP_LOGI(TAG, "Departure interval: %u minutes (%u seconds), Next departure update: %u",
             config.transportInterval, intervalSeconds, nextUpdate);

    return nextUpdate;
}

uint32_t TimingManager::findNearestUpdateTime(uint32_t weather, uint32_t transport, uint32_t ota) {
    uint32_t nearest = 0;

    // Collect all valid update times
    if (weather > 0) nearest = weather;
    if (transport > 0 && (nearest == 0 || transport < nearest)) nearest = transport;
    if (ota > 0 && (nearest == 0 || ota < nearest)) nearest = ota;

    // Fallback if no updates scheduled
    if (nearest == 0) {
        time_t now = GET_CURRENT_TIME();
        nearest = (uint32_t)now + 60; // Wake in 1 minute
        ESP_LOGI(TAG, "No updates configured - fallback wake in 60 seconds at: %u", nearest);
        return nearest;
    }

    // Log what we found
    if (weather > 0 && transport > 0 && ota > 0) {
        ESP_LOGI(TAG, "All updates needed - nearest at: %u seconds", nearest);
    } else if (weather > 0 && transport > 0) {
        ESP_LOGI(TAG, "Weather and departure updates needed - nearest at: %u seconds", nearest);
    } else if (weather > 0 && ota > 0) {
        ESP_LOGI(TAG, "Weather and OTA updates needed - nearest at: %u seconds", nearest);
    } else if (transport > 0 && ota > 0) {
        ESP_LOGI(TAG, "Departure and OTA updates needed - nearest at: %u seconds", nearest);
    } else if (transport > 0) {
        ESP_LOGI(TAG, "Only departure update needed at: %u seconds", nearest);
    } else if (weather > 0) {
        ESP_LOGI(TAG, "Only weather update needed at: %u seconds", nearest);
    } else {
        ESP_LOGI(TAG, "Only OTA update needed at: %u seconds", nearest);
    }

    return nearest;
}

uint32_t TimingManager::adjustForTransportActiveHours(uint32_t nearestUpdate, uint32_t nextTransport,
                                                      uint32_t nextWeather, uint32_t nextOTA,
                                                      uint32_t currentTime, bool& isOTAUpdate) {
    // Only adjust if the nearest update is a transport update
    if (nextTransport == 0 || nearestUpdate != nextTransport) {
        return nearestUpdate;
    }

    RTCConfigData& config = ConfigManager::getConfig();

    // Get time info for nearest update
    struct tm timeInfo;
    time_t updateTime = (time_t)nearestUpdate;
    localtime_r(&updateTime, &timeInfo);
    int updateMinutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;

    // Determine if this is weekend time
    bool isWeekend = isWeekendTime(updateTime);

    // Get appropriate transport active hours
    String activeStart = isWeekend ? String(config.weekendTransportStart) : String(config.transportActiveStart);
    String activeEnd = isWeekend ? String(config.weekendTransportEnd) : String(config.transportActiveEnd);

    int activeStartMin = parseTimeString(activeStart);
    int activeEndMin = parseTimeString(activeEnd);

    bool isActive = isTimeInRange(updateMinutes, activeStartMin, activeEndMin);

    // Special case: boundary check
    if (isActive && updateMinutes == activeEndMin && getLastTransportUpdate() == 0) {
        isActive = false;
        ESP_LOGI(TAG, "At end boundary of active hours with no previous update - treating as inactive");
    }

    if (isActive) {
        return nearestUpdate; // No adjustment needed
    }

    // Transport is inactive - calculate next active period
    ESP_LOGI(TAG, "Departure update outside transport active hours");

    struct tm currentTm;
    time_t currentTimeT = (time_t)currentTime;
    localtime_r(&currentTimeT, &currentTm);
    int currentMinutes = currentTm.tm_hour * 60 + currentTm.tm_min;
    bool isCurrentWeekend = config.weekendMode && (currentTm.tm_wday == 0 || currentTm.tm_wday == 6);

    // Get next active start time
    String nextActiveStart = isCurrentWeekend
                                 ? String(config.weekendTransportStart)
                                 : String(config.transportActiveStart);
    int nextActiveStartMin = parseTimeString(nextActiveStart);

    uint32_t nextActiveSeconds;
    if (currentMinutes < nextActiveStartMin) {
        // Active period starts later today
        int minutesUntil = nextActiveStartMin - currentMinutes;
        nextActiveSeconds = currentTime + (minutesUntil * 60);
    } else {
        // Active period starts tomorrow
        int minutesUntilMidnight = (24 * 60) - currentMinutes;
        int nextDayOfWeek = (currentTm.tm_wday + 1) % 7;
        bool isTomorrowWeekend = config.weekendMode && (nextDayOfWeek == 0 || nextDayOfWeek == 6);

        nextActiveStart = isTomorrowWeekend
                              ? String(config.weekendTransportStart)
                              : String(config.transportActiveStart);
        nextActiveStartMin = parseTimeString(nextActiveStart);
        nextActiveSeconds = currentTime + ((minutesUntilMidnight + nextActiveStartMin) * 60);
    }

    ESP_LOGI(TAG, "Next transport active period starts at: %u seconds", nextActiveSeconds);

    // Choose earliest of: next active period, weather, or OTA (OTA bypasses transport restrictions)
    uint32_t candidateWake = nextActiveSeconds;
    if (nextWeather > 0) candidateWake = min(candidateWake, nextWeather);
    if (nextOTA > 0) candidateWake = min(candidateWake, nextOTA);

    // Update isOTAUpdate flag if OTA became nearest
    isOTAUpdate = (nextOTA > 0 && candidateWake == nextOTA);

    return candidateWake;
}

uint32_t TimingManager::adjustForSleepPeriod(uint32_t nearestUpdate, uint32_t currentTime, bool isOTAUpdate) {
    RTCConfigData& config = ConfigManager::getConfig();

    // Get update time info
    struct tm updateTimeInfo;
    time_t updateTime = (time_t)nearestUpdate;
    localtime_r(&updateTime, &updateTimeInfo);
    int updateMinutes = updateTimeInfo.tm_hour * 60 + updateTimeInfo.tm_min;

    // Determine if update time is weekend
    bool isUpdateWeekend = isWeekendTime(updateTime);

    // Get sleep period configuration
    String sleepStart = isUpdateWeekend ? String(config.weekendSleepStart) : String(config.sleepStart);
    String sleepEnd = isUpdateWeekend ? String(config.weekendSleepEnd) : String(config.sleepEnd);

    int sleepStartMin = parseTimeString(sleepStart);
    int sleepEndMin = parseTimeString(sleepEnd);

    // Check if update is in sleep period
    if (!isTimeInRange(updateMinutes, sleepStartMin, sleepEndMin)) {
        return nearestUpdate; // Not in sleep period
    }

    ESP_LOGI(TAG, "Next update (%d:%02d) falls within sleep period (%s - %s)",
             updateTimeInfo.tm_hour, updateTimeInfo.tm_min, sleepStart.c_str(), sleepEnd.c_str());

    // OTA updates bypass sleep period
    if (isOTAUpdate) {
        ESP_LOGI(TAG, "OTA update scheduled during sleep period - bypassing sleep restrictions");
        return nearestUpdate;
    }

    // Calculate sleep end time
    uint32_t sleepEndSeconds;
    if (sleepEndMin > updateMinutes) {
        sleepEndSeconds = nearestUpdate + ((sleepEndMin - updateMinutes) * 60);
    } else {
        int minutesUntilNextDay = (24 * 60) - updateMinutes;
        sleepEndSeconds = nearestUpdate + ((minutesUntilNextDay + sleepEndMin) * 60);
    }

    // Check if sleep crosses weekend boundary
    struct tm sleepEndTimeInfo;
    time_t sleepEndTime = (time_t)sleepEndSeconds;
    localtime_r(&sleepEndTime, &sleepEndTimeInfo);
    bool isSleepEndWeekend = isWeekendTime(sleepEndTime);

    if (isSleepEndWeekend != isUpdateWeekend) {
        ESP_LOGI(TAG, "Sleep crosses weekend boundary - adjusting sleep end time");

        String correctSleepEnd = isSleepEndWeekend ? String(config.weekendSleepEnd) : String(config.sleepEnd);
        int correctSleepEndMin = parseTimeString(correctSleepEnd);

        if (correctSleepEndMin > updateMinutes) {
            sleepEndSeconds = nearestUpdate + ((correctSleepEndMin - updateMinutes) * 60);
        } else {
            int minutesUntilNextDay = (24 * 60) - updateMinutes;
            sleepEndSeconds = nearestUpdate + ((minutesUntilNextDay + correctSleepEndMin) * 60);
        }

        ESP_LOGI(TAG, "Adjusted sleep end to %s time: %u seconds",
                 isSleepEndWeekend ? "weekend" : "weekday", sleepEndSeconds);
    }

    ESP_LOGI(TAG, "Final wake time: %u seconds", sleepEndSeconds);
    return sleepEndSeconds;
}

// ============================================================================
// Main Sleep Duration Calculation
// ============================================================================

uint64_t TimingManager::getNextSleepDurationSeconds() {
    // Get current time and configuration
    time_t now = GET_CURRENT_TIME();
    uint32_t currentTimeSeconds = (uint32_t)now;
    RTCConfigData& config = ConfigManager::getConfig();
    uint8_t displayMode = config.displayMode;

    ESP_LOGI(TAG, "Calculating sleep duration - Display mode: %d, Current time: %u", displayMode, currentTimeSeconds);

    // ===== HANDLE TEMPORARY MODE =====
    if (config.inTemporaryMode) {
        ESP_LOGI(TAG, "Temporary mode active - mode: %d, activated at: %u",
                 config.temporaryDisplayMode, config.temporaryModeActivationTime);

        // Calculate elapsed time since temp mode activation
        int elapsed = currentTimeSeconds - config.temporaryModeActivationTime;
        const int TEMP_MODE_DURATION = 120; // 2 minutes
        int remaining = TEMP_MODE_DURATION - elapsed;

        // Check if currently in deep sleep period
        struct tm timeInfo;
        localtime_r(&now, &timeInfo);
        int currentMinutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;
        bool isCurrentWeekend = config.weekendMode && (timeInfo.tm_wday == 0 || timeInfo.tm_wday == 6);

        String sleepStart = isCurrentWeekend ? String(config.weekendSleepStart) : String(config.sleepStart);
        String sleepEnd = isCurrentWeekend ? String(config.weekendSleepEnd) : String(config.sleepEnd);
        int sleepStartMin = parseTimeString(sleepStart);
        int sleepEndMin = parseTimeString(sleepEnd);
        bool inDeepSleepPeriod = isTimeInRange(currentMinutes, sleepStartMin, sleepEndMin);
        if (remaining > 0 && !inDeepSleepPeriod) {
            // Still showing temp mode during active hours - wait for 2 minutes to complete
            ESP_LOGI(TAG, "Temp mode: %d seconds remaining in active hours", remaining);
            return (uint64_t)max(30, remaining);
        } else if (inDeepSleepPeriod) {
            // In deep sleep period - stay in temp mode until sleep ends
            int minutesUntilSleepEnd;
            if (sleepEndMin > currentMinutes) {
                minutesUntilSleepEnd = sleepEndMin - currentMinutes;
            } else {
                minutesUntilSleepEnd = (24 * 60) - currentMinutes + sleepEndMin;
            }
            uint32_t sleepDuration = minutesUntilSleepEnd * 60;
            ESP_LOGI(TAG, "Temp mode: staying active until deep sleep end (%d seconds)", sleepDuration);
            return (uint64_t)max(30, (int)sleepDuration);
        } else {
            // 2 minutes complete and in active hours
            // Temp mode should already be cleared by ButtonManager::handleWakeupMode()
            // This path is a fallback that shouldn't normally be reached
            ESP_LOGW(TAG, "Temp mode still active in sleep calculator after 2 minutes");
            ESP_LOGW(TAG, "Flag should have been cleared by button manager - falling through to normal mode");

            // Fall through to normal configured mode calculation below
        }
    }

    // ===== NORMAL CONFIGURED MODE =====
    ESP_LOGI(TAG, "Last updates - Weather: %u seconds, Departure: %u seconds",
             getLastWeatherUpdate(), getLastTransportUpdate());

    // Step 1: Calculate next update times for each type
    uint32_t nextWeatherUpdate = calculateNextWeatherUpdate(currentTimeSeconds, displayMode);
    uint32_t nextTransportUpdate = calculateNextTransportUpdate(currentTimeSeconds, displayMode);
    uint32_t nextOTACheck = calculateNextOTACheckTime(currentTimeSeconds);

    // Step 2: Find the nearest update time
    uint32_t nearestUpdate = findNearestUpdateTime(nextWeatherUpdate, nextTransportUpdate, nextOTACheck);
    bool isOTAUpdate = (nextOTACheck > 0 && nearestUpdate == nextOTACheck);

    // Step 3: Adjust for transport active hours (if nearest is transport)
    nearestUpdate = adjustForTransportActiveHours(nearestUpdate, nextTransportUpdate,
                                                  nextWeatherUpdate, nextOTACheck,
                                                  currentTimeSeconds, isOTAUpdate);

    // Step 4: Adjust for sleep period (OTA bypasses sleep)
    nearestUpdate = adjustForSleepPeriod(nearestUpdate, currentTimeSeconds, isOTAUpdate);

    // Step 5: Calculate final sleep duration with minimum threshold
    uint64_t sleepDurationSeconds;
    if (nearestUpdate > currentTimeSeconds) {
        sleepDurationSeconds = (uint64_t)(nearestUpdate - currentTimeSeconds);
    } else {
        sleepDurationSeconds = 30; // Minimum if time is in the past
    }

    // Apply minimum sleep duration
    if (sleepDurationSeconds < 30) {
        sleepDurationSeconds = 30;
        ESP_LOGI(TAG, "Applied minimum sleep duration: 30 seconds");
    }

    ESP_LOGI(TAG, "Final sleep duration: %llu seconds (%llu minutes)",
             sleepDurationSeconds, sleepDurationSeconds / 60);

    return sleepDurationSeconds;
}

bool TimingManager::isTransportActiveTime() {
    int currentMinutes = getCurrentMinutesSinceMidnight();

    String activeStart, activeEnd;
    if (isWeekend()) {
        activeStart = ConfigManager::getWeekendTransportStart();
        activeEnd = ConfigManager::getWeekendTransportEnd();
    } else {
        activeStart = ConfigManager::getTransportActiveStart();
        activeEnd = ConfigManager::getTransportActiveEnd();
    }

    int startMinutes = parseTimeString(activeStart);
    int endMinutes = parseTimeString(activeEnd);

    return isTimeInRange(currentMinutes, startMinutes, endMinutes);
}

bool TimingManager::isWeekend() {
    RTCConfigData& config = ConfigManager::getConfig();
    if (!config.weekendMode) {
        return false; // Weekend mode disabled
    }

    time_t now = GET_CURRENT_TIME();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // tm_wday: 0 = Sunday, 6 = Saturday
    return (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
}

void TimingManager::markWeatherUpdated() {
    time_t now = GET_CURRENT_TIME();
    setLastWeatherUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Weather update timestamp recorded: %u", (uint32_t)now);
}

void TimingManager::markTransportUpdated() {
    time_t now = GET_CURRENT_TIME();
    setLastTransportUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Transport update timestamp recorded: %u", (uint32_t)now);
}

bool TimingManager::isTimeForWeatherUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();

    if (lastUpdate == 0) {
        ESP_LOGI(TAG, "No previous weather update - update required");
        return true; // No previous update
    }

    time_t now = GET_CURRENT_TIME();
    uint32_t currentTime = (uint32_t)now;

    uint32_t intervalSeconds = config.weatherInterval * 3600; // Convert hours to seconds
    bool needUpdate = (currentTime - lastUpdate) >= intervalSeconds;

    ESP_LOGI(TAG, "Weather: last=%u, now=%u, interval=%u hours, need_update=%s",
             lastUpdate, currentTime, config.weatherInterval, needUpdate ? "YES" : "NO");

    return needUpdate;
}

bool TimingManager::isTimeForTransportUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastTransportUpdate();

    if (lastUpdate == 0) {
        ESP_LOGI(TAG, "No previous transport update - update required");
        return true; // No previous update
    }

    time_t now = GET_CURRENT_TIME();
    uint32_t currentTime = (uint32_t)now;

    uint32_t intervalSeconds = config.transportInterval * 60; // Convert minutes to seconds
    bool needUpdate = (currentTime - lastUpdate) >= intervalSeconds;

    ESP_LOGI(TAG, "Transport: last=%u, now=%u, interval=%u minutes, need_update=%s",
             lastUpdate, currentTime, config.transportInterval, needUpdate ? "YES" : "NO");

    return needUpdate;
}

// Private helper functions
int TimingManager::parseTimeString(const String& timeStr) {
    // Parse "HH:MM" format to minutes since midnight
    int colonPos = timeStr.indexOf(':');
    if (colonPos == -1) return 0;
    int hours = timeStr.substring(0, colonPos).toInt();
    int minutes = timeStr.substring(colonPos + 1).toInt();

    return hours * 60 + minutes;
}

int TimingManager::getCurrentMinutesSinceMidnight() {
    time_t now = GET_CURRENT_TIME();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

bool TimingManager::isTimeInRange(int currentMinutes, int startMinutes, int endMinutes) {
    if (startMinutes <= endMinutes) {
        // Same day range (e.g., 06:00 to 22:00)
        return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
    } else {
        // Overnight range (e.g., 22:30 to 05:30)
        return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
    }
}

uint32_t TimingManager::getLastWeatherUpdate() {
    return lastWeatherUpdate;
}

uint32_t TimingManager::getLastTransportUpdate() {
    return lastTransportUpdate;
}

void TimingManager::setLastWeatherUpdate(uint32_t timestamp) {
    lastWeatherUpdate = timestamp;
}

void TimingManager::setLastTransportUpdate(uint32_t timestamp) {
    lastTransportUpdate = timestamp;
}

uint32_t TimingManager::getLastOTACheck() {
    return lastOTACheck;
}

void TimingManager::setLastOTACheck(uint32_t timestamp) {
    lastOTACheck = timestamp;
}

uint32_t TimingManager::calculateNextOTACheckTime(uint32_t currentTimeSeconds) {
    RTCConfigData& config = ConfigManager::getConfig();

    // Check if OTA is enabled
    if (!config.otaEnabled) {
        ESP_LOGD(TAG, "OTA automatic updates are disabled");
        return 0; // Return 0 to indicate OTA is not scheduled
    }

    // Check if we already checked OTA recently (within last 2 minutes to avoid repeated checks)
    if (lastOTACheck > 0 && (currentTimeSeconds - lastOTACheck) < 120) {
        ESP_LOGD(TAG, "OTA check already performed recently (within 2 minutes)");
        return 0; // Skip OTA check
    }

    // Parse configured OTA check time (format: "HH:MM")
    int otaCheckMinutes = parseTimeString(String(config.otaCheckTime));

    // Get current time info
    time_t currentTime = (time_t)currentTimeSeconds;
    struct tm currentTm;
    localtime_r(&currentTime, &currentTm);
    int currentMinutes = currentTm.tm_hour * 60 + currentTm.tm_min;

    // Calculate next OTA check time
    uint32_t nextOTACheckSeconds = 0;

    if (currentMinutes < otaCheckMinutes) {
        // OTA check time is later today
        int minutesUntilOTA = otaCheckMinutes - currentMinutes;
        nextOTACheckSeconds = currentTimeSeconds + (minutesUntilOTA * 60);
        ESP_LOGD(TAG, "Next OTA check is later today in %d minutes at %02d:%02d",
                 minutesUntilOTA, otaCheckMinutes / 60, otaCheckMinutes % 60);
    } else {
        // OTA check time is tomorrow
        int minutesUntilMidnight = (24 * 60) - currentMinutes;
        int minutesUntilOTA = minutesUntilMidnight + otaCheckMinutes;
        nextOTACheckSeconds = currentTimeSeconds + (minutesUntilOTA * 60);
        ESP_LOGD(TAG, "Next OTA check is tomorrow in %d minutes at %02d:%02d",
                 minutesUntilOTA, otaCheckMinutes / 60, otaCheckMinutes % 60);
    }

    ESP_LOGI(TAG, "Next OTA check scheduled at: %u seconds", nextOTACheckSeconds);
    return nextOTACheckSeconds;
}

