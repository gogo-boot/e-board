#include "util/timing_manager.h"
#include "util/time_manager.h"
#include <esp_log.h>
#include <time.h>

static const char* TAG = "TIMING_MGR";

// RTC memory for storing last update timestamps
RTC_DATA_ATTR uint32_t lastWeatherUpdate = 0;
RTC_DATA_ATTR uint32_t lastTransportUpdate = 0;

UpdateType TimingManager::getRequiredUpdates() {
    bool needWeather = isTimeForWeatherUpdate();
    bool needTransport = isTimeForTransportUpdate();

    // Check if we're in active transport hours
    if (!isTransportActiveTime()) {
        needTransport = false;
        ESP_LOGI(TAG, "Outside transport active hours - skipping transport update");
    }

    if (needWeather && needTransport) {
        ESP_LOGI(TAG, "Both weather and transport updates required");
        return UpdateType::BOTH;
    } else if (needWeather) {
        ESP_LOGI(TAG, "Weather update required");
        return UpdateType::WEATHER_ONLY;
    } else if (needTransport) {
        ESP_LOGI(TAG, "Transport update required");
        return UpdateType::TRANSPORT_ONLY;
    }

    ESP_LOGI(TAG, "No updates required");
    return UpdateType::WEATHER_ONLY; // Default fallback
}

uint64_t TimingManager::getNextSleepDurationSeconds() {
    // Check if we're in sleep hours
    TimeOfDay timeStatus = getCurrentTimeStatus();

    if (timeStatus == TimeOfDay::SLEEP_HOURS) {
        // Calculate time until sleep period ends
        String sleepEnd = isWeekend() ? ConfigManager::getWeekendSleepEnd() : ConfigManager::getSleepEnd();
        int sleepEndMinutes = parseTimeString(sleepEnd);
        int currentMinutes = getCurrentMinutesSinceMidnight();

        int minutesUntilWake;
        if (sleepEndMinutes > currentMinutes) {
            minutesUntilWake = sleepEndMinutes - currentMinutes;
        } else {
            // Sleep end is next day
            minutesUntilWake = (24 * 60) - currentMinutes + sleepEndMinutes;
        }

        ESP_LOGI(TAG, "In sleep hours - sleeping for %d minutes", minutesUntilWake);
        return (uint64_t)minutesUntilWake * 60 * 1000000ULL; // Convert to microseconds
    }

    // In active hours - determine next update interval
    int minutesUntilNext = getMinutesUntilNextUpdate();

    // Minimum sleep time is 1 minute, maximum is configured interval
    minutesUntilNext = max(1, minutesUntilNext);

    ESP_LOGI(TAG, "Next update in %d minutes", minutesUntilNext);
    return (uint64_t)minutesUntilNext * 60; // Convert to seconds
}

TimeOfDay TimingManager::getCurrentTimeStatus() {
    int currentMinutes = getCurrentMinutesSinceMidnight();

    String sleepStart, sleepEnd;
    if (isWeekend()) {
        sleepStart = ConfigManager::getWeekendSleepStart();
        sleepEnd = ConfigManager::getWeekendSleepEnd();
    } else {
        sleepStart = ConfigManager::getSleepStart();
        sleepEnd = ConfigManager::getSleepEnd();
    }

    int sleepStartMinutes = parseTimeString(sleepStart);
    int sleepEndMinutes = parseTimeString(sleepEnd);

    if (isTimeInRange(currentMinutes, sleepStartMinutes, sleepEndMinutes)) {
        return TimeOfDay::SLEEP_HOURS;
    }

    return TimeOfDay::ACTIVE_HOURS;
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

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // tm_wday: 0 = Sunday, 6 = Saturday
    return (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6);
}

void TimingManager::markWeatherUpdated() {
    time_t now;
    time(&now);
    setLastWeatherUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Weather update timestamp recorded: %u", (uint32_t)now);
}

void TimingManager::markTransportUpdated() {
    time_t now;
    time(&now);
    setLastTransportUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Transport update timestamp recorded: %u", (uint32_t)now);
}

// currentTime, lastWeather, lastTransport: seconds since Unix epoch (timestamps)
// weatherInterval: minutes (converted from hours earlier in the function)
// transportInterval: minutes
// minutesSinceWeather, minutesSinceTransport: minutes
// minutesUntilWeather, minutesUntilTransport: minutes
int TimingManager::getMinutesUntilNextUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();

    int weatherInterval = config.weatherInterval * 60; // Convert hours to minutes
    int transportInterval = config.transportInterval; // Already in minutes

    time_t now;
    time(&now);
    uint32_t currentTime = (uint32_t)now;

    uint32_t lastWeather = getLastWeatherUpdate();
    uint32_t lastTransport = getLastTransportUpdate();

    int minutesSinceWeather = lastWeather > 0 ? (currentTime - lastWeather) / 60 : weatherInterval;
    int minutesSinceTransport = lastTransport > 0 ? (currentTime - lastTransport) / 60 : transportInterval;

    int minutesUntilWeather = max(0, weatherInterval - minutesSinceWeather);
    int minutesUntilTransport = max(0, transportInterval - minutesSinceTransport);

    // If transport is not in active hours, ignore transport timing
    if (!isTransportActiveTime()) {
        return minutesUntilWeather;
    }

    // Return the shorter interval (next required update)
    return min(minutesUntilWeather, minutesUntilTransport);
}

int TimingManager::getEarliestDepartureTime() {
    RTCConfigData& config = ConfigManager::getConfig();
    return config.walkingTime; // Walking time in minutes from now
}

bool TimingManager::isTimeForWeatherUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();

    if (lastUpdate == 0) {
        ESP_LOGI(TAG, "No previous weather update - update required");
        return true; // No previous update
    }

    time_t now;
    time(&now);
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

    time_t now;
    time(&now);
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
    time_t now;
    time(&now);
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
