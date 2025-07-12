#include "util/time_manager.h"
#include <esp_log.h>
#include <time.h>

static const char* TAG = "TIME_MGR";

void TimeManager::setupNTPTime() {
    // Set timezone to Central European Time (CET/CEST) - Germany
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    ESP_LOGI(TAG, "Waiting for NTP time sync (German timezone)");
    time_t now = time(nullptr);
    int retry = 0;
    const int retry_count = 30;
    while (now < 8 * 3600 * 2 && retry < retry_count) { // year < 1971
        delay(500);
        ESP_LOGD(TAG, ".");
        now = time(nullptr);
        retry++;
    }
    
    if (retry >= retry_count) {
        ESP_LOGW(TAG, "Failed to sync NTP time after %d attempts", retry_count);
        return;
    }
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "NTP time set (German time): %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void TimeManager::printCurrentTime() {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

bool TimeManager::isTimeSet() {
    time_t now = time(nullptr);
    return now > 8 * 3600 * 2; // Check if year > 1971
}
