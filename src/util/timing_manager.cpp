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

uint64_t TimingManager::getNextSleepDurationSeconds() {
    // Get current time in seconds since Unix epoch
    time_t now = GET_CURRENT_TIME();
    uint32_t currentTimeSeconds = (uint32_t)now;

    // Get configuration
    RTCConfigData& config = ConfigManager::getConfig();
    uint8_t displayMode = config.displayMode; // 0=half_and_half, 1=weather_only, 2=departure_only

    ESP_LOGI(TAG, "Calculating sleep duration - Display mode: %d, Current time: %u", displayMode, currentTimeSeconds);

    // Get last update times in seconds
    uint32_t lastWeatherUpdateSeconds = getLastWeatherUpdate();
    uint32_t lastDepartureUpdateSeconds = getLastTransportUpdate();

    ESP_LOGI(TAG, "Last updates - Weather: %u seconds, Departure: %u seconds",
             lastWeatherUpdateSeconds, lastDepartureUpdateSeconds);

    // Calculate next update times based on display mode
    uint32_t nextWeatherUpdateSeconds = 0;
    uint32_t nextDepartureUpdateSeconds = 0;

    // Weather updates needed for weather_only (1) and half_and_half (0) modes
    if (displayMode == 0 || displayMode == 1) {
        uint32_t weatherIntervalSeconds = config.weatherInterval * 3600; // hours to seconds
        if (lastWeatherUpdateSeconds == 0) {
            nextWeatherUpdateSeconds = currentTimeSeconds; // Update immediately if never updated
        } else {
            nextWeatherUpdateSeconds = lastWeatherUpdateSeconds + weatherIntervalSeconds;
        }
        ESP_LOGI(TAG, "Weather interval: %u hours (%u seconds), Next weather update: %u",
                 config.weatherInterval, weatherIntervalSeconds, nextWeatherUpdateSeconds);
    }

    // Departure updates needed for departure_only (2) and half_and_half (0) modes
    if (displayMode == 0 || displayMode == 2) {
        uint32_t departureIntervalSeconds = config.transportInterval * 60; // minutes to seconds
        if (lastDepartureUpdateSeconds == 0) {
            nextDepartureUpdateSeconds = currentTimeSeconds; // Update immediately if never updated
        } else {
            nextDepartureUpdateSeconds = lastDepartureUpdateSeconds + departureIntervalSeconds;
        }
        ESP_LOGI(TAG, "Departure interval: %u minutes (%u seconds), Next departure update: %u",
                 config.transportInterval, departureIntervalSeconds, nextDepartureUpdateSeconds);
    }

    // Calculate next OTA check time (independent of display mode)
    uint32_t nextOTACheckSeconds = calculateNextOTACheckTime(currentTimeSeconds);

    // Find the nearest next update time
    uint32_t nearestUpdateSeconds = 0;
    if (nextWeatherUpdateSeconds > 0 && nextDepartureUpdateSeconds > 0 && nextOTACheckSeconds > 0) {
        nearestUpdateSeconds = min(min(nextWeatherUpdateSeconds, nextDepartureUpdateSeconds), nextOTACheckSeconds);
        ESP_LOGI(TAG, "All updates needed - nearest at: %u seconds", nearestUpdateSeconds);
    } else if (nextWeatherUpdateSeconds > 0 && nextDepartureUpdateSeconds > 0) {
        nearestUpdateSeconds = min(nextWeatherUpdateSeconds, nextDepartureUpdateSeconds);
        ESP_LOGI(TAG, "Weather and departure updates needed - nearest at: %u seconds", nearestUpdateSeconds);
    } else if (nextWeatherUpdateSeconds > 0 && nextOTACheckSeconds > 0) {
        nearestUpdateSeconds = min(nextWeatherUpdateSeconds, nextOTACheckSeconds);
        ESP_LOGI(TAG, "Weather and OTA updates needed - nearest at: %u seconds", nearestUpdateSeconds);
    } else if (nextDepartureUpdateSeconds > 0 && nextOTACheckSeconds > 0) {
        nearestUpdateSeconds = min(nextDepartureUpdateSeconds, nextOTACheckSeconds);
        ESP_LOGI(TAG, "Departure and OTA updates needed - nearest at: %u seconds", nearestUpdateSeconds);
    } else if (nextDepartureUpdateSeconds > 0) {
        nearestUpdateSeconds = nextDepartureUpdateSeconds;
        ESP_LOGI(TAG, "Only departure update needed at: %u seconds", nearestUpdateSeconds);
    } else if (nextWeatherUpdateSeconds > 0) {
        nearestUpdateSeconds = nextWeatherUpdateSeconds;
        ESP_LOGI(TAG, "Only weather update needed at: %u seconds", nearestUpdateSeconds);
    } else if (nextOTACheckSeconds > 0) {
        nearestUpdateSeconds = nextOTACheckSeconds;
        ESP_LOGI(TAG, "Only OTA update needed at: %u seconds", nearestUpdateSeconds);
    } else {
        // Fallback - wake up in 1 minute
        nearestUpdateSeconds = currentTimeSeconds + 60;
        ESP_LOGI(TAG, "No updates configured - fallback wake in 60 seconds at: %u", nearestUpdateSeconds);
    }

    // Check if nearest update is OTA (to bypass sleep period restrictions)
    bool isOTAUpdate = (nextOTACheckSeconds > 0 && nearestUpdateSeconds == nextOTACheckSeconds);

    // Check if departure update is during transport active hours
    if (nextDepartureUpdateSeconds > 0 && nearestUpdateSeconds == nextDepartureUpdateSeconds) {
        // Convert nearestUpdateSeconds to time of day to check transport active hours
        struct tm timeinfo;
        time_t nearestTime = (time_t)nearestUpdateSeconds;
        localtime_r(&nearestTime, &timeinfo);
        int nearestMinutesSinceMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;

        // Get transport active hours
        String activeStart, activeEnd;
        bool isWeekendTime = false;
        if (config.weekendMode) {
            time_t nearestTimeT = (time_t)nearestUpdateSeconds;
            struct tm nearestTm;
            localtime_r(&nearestTimeT, &nearestTm);
            isWeekendTime = (nearestTm.tm_wday == 0 || nearestTm.tm_wday == 6);
        }

        if (isWeekendTime) {
            activeStart = String(config.weekendTransportStart);
            activeEnd = String(config.weekendTransportEnd);
        } else {
            activeStart = String(config.transportActiveStart);
            activeEnd = String(config.transportActiveEnd);
        }

        int activeStartMinutes = parseTimeString(activeStart);
        int activeEndMinutes = parseTimeString(activeEnd);

        bool isInActiveHours = isTimeInRange(nearestMinutesSinceMidnight, activeStartMinutes, activeEndMinutes);

        // Special case: if at the exact end boundary and transport interval hasn't passed yet,
        // treat it as outside active hours
        if (isInActiveHours && nearestMinutesSinceMidnight == activeEndMinutes &&
            lastDepartureUpdateSeconds == 0) {
            isInActiveHours = false;
            ESP_LOGI(TAG, "At end boundary of active hours with no previous update - treating as inactive");
        }

        if (!isInActiveHours) {
            ESP_LOGI(TAG, "Departure update outside transport active hours");

            // Calculate next transport active period start time
            // We need to find when transport will be active next
            struct tm currentTm;
            time_t currentTimeT = (time_t)currentTimeSeconds;
            localtime_r(&currentTimeT, &currentTm);
            int currentMinutes = currentTm.tm_hour * 60 + currentTm.tm_min;
            bool isCurrentWeekend = config.weekendMode && (currentTm.tm_wday == 0 || currentTm.tm_wday == 6);

            String nextActiveStart;
            int nextActiveStartMinutes;
            uint32_t nextActiveSeconds = 0;

            // Check if we're currently in a weekend or weekday
            if (isCurrentWeekend) {
                nextActiveStart = String(config.weekendTransportStart);
            } else {
                nextActiveStart = String(config.transportActiveStart);
            }
            nextActiveStartMinutes = parseTimeString(nextActiveStart);

            // Calculate seconds until next active period
            if (currentMinutes < nextActiveStartMinutes) {
                // Active period starts later today
                int minutesUntilActive = nextActiveStartMinutes - currentMinutes;
                nextActiveSeconds = currentTimeSeconds + (minutesUntilActive * 60);
            } else {
                // Active period starts tomorrow
                int minutesUntilMidnight = (24 * 60) - currentMinutes;

                // Determine if tomorrow is weekend or weekday
                int nextDayOfWeek = (currentTm.tm_wday + 1) % 7;
                bool isTomorrowWeekend = config.weekendMode && (nextDayOfWeek == 0 || nextDayOfWeek == 6);

                if (isTomorrowWeekend) {
                    nextActiveStart = String(config.weekendTransportStart);
                } else {
                    nextActiveStart = String(config.transportActiveStart);
                }
                nextActiveStartMinutes = parseTimeString(nextActiveStart);
                nextActiveSeconds = currentTimeSeconds + ((minutesUntilMidnight + nextActiveStartMinutes) * 60);
            }

            ESP_LOGI(TAG, "Next transport active period starts at: %u seconds", nextActiveSeconds);

            // Choose the earlier of: next weather update or next transport active period
            if (nextWeatherUpdateSeconds > 0) {
                nearestUpdateSeconds = min(nextWeatherUpdateSeconds, nextActiveSeconds);
            } else {
                nearestUpdateSeconds = nextActiveSeconds;
            }
        }
    }

    // Check if nearest update time falls within configured deep sleep period
    struct tm updateTimeInfo;
    time_t updateTime = (time_t)nearestUpdateSeconds;
    localtime_r(&updateTime, &updateTimeInfo);
    int updateMinutesSinceMidnight = updateTimeInfo.tm_hour * 60 + updateTimeInfo.tm_min;

    // Determine if update time is weekend
    bool isUpdateWeekend = false;
    if (config.weekendMode) {
        isUpdateWeekend = (updateTimeInfo.tm_wday == 0 || updateTimeInfo.tm_wday == 6);
    }

    // Get sleep period configuration based on update time's day
    String sleepStart, sleepEnd;
    if (isUpdateWeekend) {
        sleepStart = String(config.weekendSleepStart);
        sleepEnd = String(config.weekendSleepEnd);
    } else {
        sleepStart = String(config.sleepStart);
        sleepEnd = String(config.sleepEnd);
    }

    int sleepStartMinutes = parseTimeString(sleepStart);
    int sleepEndMinutes = parseTimeString(sleepEnd);

    // Check if update time is in sleep period
    bool isUpdateInSleepPeriod = isTimeInRange(updateMinutesSinceMidnight, sleepStartMinutes, sleepEndMinutes);

    if (isUpdateInSleepPeriod) {
        ESP_LOGI(TAG, "Next update (%d:%02d) falls within sleep period (%s - %s)",
                 updateTimeInfo.tm_hour, updateTimeInfo.tm_min, sleepStart.c_str(), sleepEnd.c_str());

        // OTA updates bypass sleep period restrictions
        if (isOTAUpdate) {
            ESP_LOGI(TAG, "OTA update scheduled during sleep period - bypassing sleep restrictions");
            // Keep nearestUpdateSeconds as is - wake for OTA even during sleep
        } else {
            // Calculate sleep end time in seconds
            uint32_t sleepEndSeconds;
            if (sleepEndMinutes > updateMinutesSinceMidnight) {
                // Sleep ends same day
                sleepEndSeconds = nearestUpdateSeconds + ((sleepEndMinutes - updateMinutesSinceMidnight) * 60);
            } else {
                // Sleep ends next day
                int minutesUntilNextDay = (24 * 60) - updateMinutesSinceMidnight;
                sleepEndSeconds = nearestUpdateSeconds + ((minutesUntilNextDay + sleepEndMinutes) * 60);
            }

            // NOW check if the sleep END time (wake-up time) is weekend or weekday
            // This is important for Friday->Saturday and Sunday->Monday transitions
            struct tm sleepEndTimeInfo;
            time_t sleepEndTime = (time_t)sleepEndSeconds;
            localtime_r(&sleepEndTime, &sleepEndTimeInfo);
            bool isSleepEndWeekend = false;
            if (config.weekendMode) {
                isSleepEndWeekend = (sleepEndTimeInfo.tm_wday == 0 || sleepEndTimeInfo.tm_wday == 6);
            }

            // If sleep end day type differs from update day type, recalculate with correct sleep end time
            if (isSleepEndWeekend != isUpdateWeekend) {
                ESP_LOGI(TAG, "Sleep crosses weekend boundary - adjusting sleep end time");

                // Get the correct sleep end time for the wake-up day
                String correctSleepEnd;
                if (isSleepEndWeekend) {
                    correctSleepEnd = String(config.weekendSleepEnd);
                } else {
                    correctSleepEnd = String(config.sleepEnd);
                }

                int correctSleepEndMinutes = parseTimeString(correctSleepEnd);
                // Recalculate sleep end time with correct configuration
                if (correctSleepEndMinutes > updateMinutesSinceMidnight) {
                    // Sleep ends same day (shouldn't happen if we crossed boundary, but check anyway)
                    sleepEndSeconds = nearestUpdateSeconds + ((correctSleepEndMinutes - updateMinutesSinceMidnight) *
                        60);
                } else {
                    // Sleep ends next day
                    int minutesUntilNextDay = (24 * 60) - updateMinutesSinceMidnight;
                    sleepEndSeconds = nearestUpdateSeconds + ((minutesUntilNextDay + correctSleepEndMinutes) * 60);
                }

                ESP_LOGI(TAG, "Adjusted sleep end to %s time: %u seconds",
                         isSleepEndWeekend ? "weekend" : "weekday", sleepEndSeconds);
            }

            nearestUpdateSeconds = sleepEndSeconds;
            ESP_LOGI(TAG, "Final wake time: %u seconds", nearestUpdateSeconds);
        } // End of else block for non-OTA updates
    }

    // Calculate sleep duration in seconds
    uint64_t sleepDurationSeconds;
    if (nearestUpdateSeconds > currentTimeSeconds) {
        sleepDurationSeconds = (uint64_t)(nearestUpdateSeconds - currentTimeSeconds);
    } else {
        // If calculated time is in the past, wake up in minimum time
        sleepDurationSeconds = 30; // 30 seconds minimum
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

