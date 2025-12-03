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


uint32_t TimingManager::calculateNextWeatherUpdate(uint32_t currentTimeSeconds) {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();
    uint32_t intervalSeconds = config.weatherInterval * 3600; // hours to seconds

    uint32_t nextUpdate = (lastUpdate == 0) ? currentTimeSeconds : lastUpdate + intervalSeconds;

    ESP_LOGI(TAG, "Weather interval: %u hours (%u seconds), Next weather update: %u",
             config.weatherInterval, intervalSeconds, nextUpdate);

    return nextUpdate;
}

uint32_t TimingManager::calculateNextTransportUpdate(uint32_t currentTimeSeconds) {
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

    // Fallback if no updates scheduled; log all parameters and nearest value
    ESP_LOGI(TAG, "findNearestUpdateTime params: weather=%u, transport=%u, ota=%u, nearest=%u", weather, transport, ota,
             nearest);

    if (nearest == 0) {
        time_t now = GET_CURRENT_TIME();
        nearest = (uint32_t)now + 60; // Wake in 1 minute
        ESP_LOGI(TAG, "No updates configured - fallback wake in 60 seconds at: %u", nearest);
        return nearest;
    }

    return nearest;
}

bool TimingManager::isTransportActiveAtTime(uint32_t timestamp) {
    RTCConfigData& config = ConfigManager::getConfig();

    tm timeInfo;
    time_t time = (time_t)timestamp;
    localtime_r(&time, &timeInfo);

    bool weekend = isWeekend(timestamp);

    String start = weekend ? String(config.weekendTransportStart) : String(config.transportActiveStart);
    String end = weekend ? String(config.weekendTransportEnd) : String(config.transportActiveEnd);

    int minutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;
    int startMin = parseTimeString(start);
    int endMin = parseTimeString(end);

    bool isActive = isTimeInRange(minutes, startMin, endMin);

    // Special case: boundary check - treat end boundary as inactive if no previous update
    if (isActive && minutes == endMin && getLastTransportUpdate() == 0) {
        ESP_LOGD(TAG, "At end boundary of active hours with no previous update - treating as inactive");
        return false;
    }

    return isActive;
}

uint32_t TimingManager::calculateNextActiveTransportTime(uint32_t currentTime) {
    RTCConfigData& config = ConfigManager::getConfig();

    struct tm currentTm;
    time_t now = (time_t)currentTime;
    localtime_r(&now, &currentTm);

    int currentMin = currentTm.tm_hour * 60 + currentTm.tm_min;
    bool isCurrentWeekend = config.weekendMode && (currentTm.tm_wday == 0 || currentTm.tm_wday == 6);

    String start = isCurrentWeekend ? String(config.weekendTransportStart) : String(config.transportActiveStart);
    int startMin = parseTimeString(start);

    uint32_t nextActiveTime;

    if (currentMin < startMin) {
        // Active period starts later today
        int minutesUntil = startMin - currentMin;
        nextActiveTime = currentTime + (minutesUntil * 60);
        ESP_LOGD(TAG, "Transport active starts in %d minutes", minutesUntil);
    } else {
        // Active period starts tomorrow
        int minutesToMidnight = (24 * 60) - currentMin;

        // Check if tomorrow is weekend
        int nextDayOfWeek = (currentTm.tm_wday + 1) % 7;
        bool isTomorrowWeekend = config.weekendMode && (nextDayOfWeek == 0 || nextDayOfWeek == 6);
        String tomorrowStart = isTomorrowWeekend
                                   ? String(config.weekendTransportStart)
                                   : String(config.transportActiveStart);
        int tomorrowStartMin = parseTimeString(tomorrowStart);

        nextActiveTime = currentTime + ((minutesToMidnight + tomorrowStartMin) * 60);
        ESP_LOGD(TAG, "Transport active starts tomorrow at %02d:%02d", tomorrowStartMin / 60, tomorrowStartMin % 60);
    }

    ESP_LOGI(TAG, "Next transport active time: %u", nextActiveTime);
    return nextActiveTime;
}

// Adjust nearest update time based on transport active hours
// If transport update is outside active hours, skip to next valid update
// (weather, OTA, or next active period)
uint32_t TimingManager::adjustForTransportActiveHours(uint32_t nearestUpdate, uint32_t nextTransport,
                                                      uint32_t nextWeather, uint32_t nextOTA,
                                                      uint32_t currentTime, bool& isOTAUpdate) {
    // Only process if nearest update is a transport update
    if (nextTransport == 0 || nearestUpdate != nextTransport) {
        return nearestUpdate; // Not a transport update - no adjustment needed
    }

    // Check if transport update falls within active hours
    if (isTransportActiveAtTime(nearestUpdate)) {
        ESP_LOGD(TAG, "Transport update is within active hours - no adjustment needed");
        return nearestUpdate; // Transport is active - keep it
    }

    // Transport is inactive - skip to next valid update
    ESP_LOGI(TAG, "Departure update outside transport active hours");

    // Find next valid update from alternatives (weather, OTA, or next active period)
    uint32_t nextValidUpdate = UINT32_MAX;

    // Check weather update
    if (nextWeather > 0 && nextWeather < nextValidUpdate) {
        nextValidUpdate = nextWeather;
        ESP_LOGD(TAG, "Alternative: weather update at %u", nextWeather);
    }

    // Check OTA update
    if (nextOTA > 0 && nextOTA < nextValidUpdate) {
        nextValidUpdate = nextOTA;
        ESP_LOGD(TAG, "Alternative: OTA update at %u", nextOTA);
    }
    // If no weather or OTA, calculate next transport active period
    if (nextValidUpdate == UINT32_MAX) {
        nextValidUpdate = calculateNextActiveTransportTime(currentTime);
        ESP_LOGI(TAG, "No weather/OTA - using next transport active period");
    }

    // Update OTA flag if OTA became the nearest
    isOTAUpdate = (nextOTA > 0 && nextValidUpdate == nextOTA);

    ESP_LOGI(TAG, "Adjusted wake time: %u (isOTA=%d)", nextValidUpdate, isOTAUpdate);
    return nextValidUpdate;
}

uint32_t TimingManager::adjustForSleepPeriod(uint32_t nearestUpdate, bool isOTAUpdate) {
    RTCConfigData& config = ConfigManager::getConfig();

    // Get update time info
    tm updateTimeInfo;
    time_t updateTime = (time_t)nearestUpdate;
    localtime_r(&updateTime, &updateTimeInfo);
    uint16_t updateMinutes = updateTimeInfo.tm_hour * 60 + updateTimeInfo.tm_min;

    uint16_t sleepStartMin = getSleepStartMin();
    uint16_t sleepEndMin = getSleepEndMin();

    // Check if update is in sleep period
    if (!isTimeInRange(updateMinutes, sleepStartMin, sleepEndMin)) {
        return nearestUpdate; // Not in sleep period
    }

    ESP_LOGI(TAG, "Next update (%d:%02d) falls within sleep period (%d - %d)",
             updateTimeInfo.tm_hour, updateTimeInfo.tm_min, sleepStartMin, sleepEndMin);

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
    tm sleepEndTimeInfo;
    time_t sleepEndTime = (time_t)sleepEndSeconds;
    localtime_r(&sleepEndTime, &sleepEndTimeInfo);

    // Determine if update time is weekend
    bool isUpdateWeekend = isWeekend(updateTime);
    bool isSleepEndWeekend = isWeekend(sleepEndTime);

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

    // Get effective display mode (considers temporary mode)
    uint8_t displayMode = getEffectiveDisplayMode();

    ESP_LOGI(
        TAG,
        "Calculating sleep duration - Effective Display mode: %d (temp=%d, configured=%d), Current time: %u",
        displayMode, config.inTemporaryMode, config.displayMode, currentTimeSeconds);

    // ===== HANDLE TEMPORARY MODE =====
    if (config.inTemporaryMode) {
        ESP_LOGI(TAG, "Temporary mode active - mode: %d, activated at: %u",
                 config.temporaryDisplayMode, config.temporaryModeActivationTime);

        // Calculate elapsed time since temp mode activation
        int elapsed = currentTimeSeconds - config.temporaryModeActivationTime;
        const int TEMP_MODE_DURATION = 120; // 2 minutes
        int remaining = TEMP_MODE_DURATION - elapsed;

        // Check if currently in deep sleep period
        int currentMinutes = getCurrentMin();
        int sleepEndMin = getSleepEndMin();
        bool inDeepSleepPeriod = isInDeepSleepPeriod();

        if (remaining > 0 && !inDeepSleepPeriod) {
            // Still showing temp mode during active hours - wait for 2 minutes to complete
            ESP_LOGI(TAG, "Temp mode: %d seconds remaining in active hours", remaining);
            return (uint64_t)max(30, remaining);
        }

        if (inDeepSleepPeriod) {
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
        }
        // 2 minutes complete and in active hours
        // Temp mode should already be cleared by ButtonManager::handleWakeupMode()
        // This path is a fallback that shouldn't normally be reached
        ESP_LOGW(TAG, "Temp mode still active in sleep calculator after 2 minutes");
        ESP_LOGW(TAG, "Flag should have been cleared by button manager - falling through to normal mode");

        // Fall through to normal configured mode calculation below
    }

    // ===== NORMAL CONFIGURED MODE =====
    ESP_LOGI(TAG, "Last updates - Weather: %u seconds, Departure: %u seconds",
             getLastWeatherUpdate(), getLastTransportUpdate());

    // Step 1: Calculate next update times for each type
    // it returns 0 if that update type is not needed
    uint32_t nextWeatherUpdate = 0;
    uint32_t nextTransportUpdate = 0;
    uint32_t nextOTACheck = calculateNextOTACheckTime(currentTimeSeconds);
    uint32_t nearestUpdate = 0;

    switch (displayMode) {
    case DISPLAY_MODE_HALF_AND_HALF:
        ESP_LOGI(TAG, "Display mode: HALF AND HALF");
        nextWeatherUpdate = calculateNextWeatherUpdate(currentTimeSeconds);
        nextTransportUpdate = calculateNextTransportUpdate(currentTimeSeconds);
        break;
    case DISPLAY_MODE_WEATHER_ONLY:
        ESP_LOGI(TAG, "Display mode: WEATHER ONLY");
        nextWeatherUpdate = calculateNextWeatherUpdate(currentTimeSeconds);
        break;
    case DISPLAY_MODE_TRANSPORT_ONLY:
        ESP_LOGI(TAG, "Display mode: TRANSPORT ONLY");
        nextTransportUpdate = calculateNextTransportUpdate(currentTimeSeconds);
        break;
    default:
        ESP_LOGI(TAG, "Unknown display mode: %d ", displayMode);
        break;
    }

    // Step 2: Find the nearest update time
    nearestUpdate = findNearestUpdateTime(nextWeatherUpdate, nextTransportUpdate, nextOTACheck);
    bool isOTAUpdate = (nextOTACheck > 0 && nearestUpdate == nextOTACheck);

    // Step 3: Adjust for transport active hours (if nearest is transport)
    nearestUpdate = adjustForTransportActiveHours(nearestUpdate, nextTransportUpdate,
                                                  nextWeatherUpdate, nextOTACheck,
                                                  currentTimeSeconds, isOTAUpdate);

    // Step 4: Adjust for sleep period (OTA bypasses sleep)
    nearestUpdate = adjustForSleepPeriod(nearestUpdate, isOTAUpdate);

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

uint16_t TimingManager::getCurrentMin() {
    time_t now = GET_CURRENT_TIME();
    tm timeInfo;
    localtime_r(&now, &timeInfo);
    return timeInfo.tm_hour * 60 + timeInfo.tm_min;;
}

uint16_t TimingManager::getSleepStartMin() {
    RTCConfigData& config = ConfigManager::getConfig();
    String sleepStart = isWeekend() ? String(config.weekendSleepStart) : String(config.sleepStart);
    return parseTimeString(sleepStart);
}

uint16_t TimingManager::getSleepEndMin() {
    RTCConfigData& config = ConfigManager::getConfig();
    String sleepEnd = isWeekend() ? String(config.weekendSleepEnd) : String(config.sleepEnd);
    return parseTimeString(sleepEnd);
}

bool TimingManager::isInDeepSleepPeriod() {
    return isTimeInRange(getCurrentMin(), getSleepStartMin(), getSleepEndMin());
}

bool TimingManager::isWeekend() {
    return isWeekend(GET_CURRENT_TIME());
}

bool TimingManager::isWeekend(time_t timestamp) {
    RTCConfigData& config = ConfigManager::getConfig();
    if (!config.weekendMode) {
        return false;
    }
    struct tm timeInfo;
    localtime_r(&timestamp, &timeInfo);

    // tm_wday: 0 = Sunday, 6 = Saturday
    return (timeInfo.tm_wday == 0 || timeInfo.tm_wday == 6);
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

uint8_t TimingManager::getEffectiveDisplayMode() {
    RTCConfigData& config = ConfigManager::getConfig();

    // If in temporary mode, use temporary display mode
    if (config.inTemporaryMode) {
        ESP_LOGD(TAG, "Using temporary display mode: %d", config.temporaryDisplayMode);
        return config.temporaryDisplayMode;
    }

    if (config.displayMode == DISPLAY_MODE_TRANSPORT_ONLY || config.displayMode == DISPLAY_MODE_WEATHER_ONLY) {
        return config.displayMode;
    } else {
        // DISPLAY_MODE_HALF_AND_HALF;
        if (isTransportActiveTime()) {
            return config.displayMode;
        } else {
            return DISPLAY_MODE_WEATHER_ONLY;
        }
    }
}

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

