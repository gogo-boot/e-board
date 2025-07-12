#include "util/time_manager.h"
#include <esp_log.h>
#include <time.h>
#include <esp_sntp.h>

static const char* TAG = "TIME_MGR";

void TimeManager::setupNTPTime() {
    ESP_LOGI(TAG, "Setting up NTP time with German timezone");
    
    // For ESP32, use configTzTime instead of configTime + setenv
    // German timezone: UTC+1 (CET) in winter, UTC+2 (CEST) in summer
    // Format: timezone_offset_seconds, daylight_offset_seconds, ntp_server
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    
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
    
    // Verify timezone is applied correctly
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "NTP time set (German time): %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        
    // Also log UTC time for comparison
    struct tm utc_timeinfo;
    gmtime_r(&now, &utc_timeinfo);
    ESP_LOGI(TAG, "UTC time: %04d-%02d-%02d %02d:%02d:%02d",
        utc_timeinfo.tm_year + 1900, utc_timeinfo.tm_mon + 1, utc_timeinfo.tm_mday,
        utc_timeinfo.tm_hour, utc_timeinfo.tm_min, utc_timeinfo.tm_sec);
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

bool TimeManager::getCurrentLocalTime(struct tm& timeinfo) {
    if (!isTimeSet()) {
        ESP_LOGW(TAG, "Time not set, cannot get current local time");
        return false;
    }
    
    time_t now = time(nullptr);
    
    // First try with localtime_r (should use timezone settings)
    localtime_r(&now, &timeinfo);
    
    // Check if timezone conversion worked by comparing with UTC
    struct tm utc_timeinfo;
    gmtime_r(&now, &utc_timeinfo);
    
    // Calculate hour difference
    int local_hour = timeinfo.tm_hour;
    int utc_hour = utc_timeinfo.tm_hour;
    int hour_diff = local_hour - utc_hour;
    
    // Handle day boundary crossing
    if (hour_diff < -12) hour_diff += 24;
    if (hour_diff > 12) hour_diff -= 24;
    
    ESP_LOGD(TAG, "UTC: %02d:%02d, Local: %02d:%02d, Diff: %+d hours", 
             utc_hour, utc_timeinfo.tm_min, local_hour, timeinfo.tm_min, hour_diff);
    
    // If timezone conversion didn't work (hour_diff is 0), manually add German offset
    if (hour_diff == 0) {
        ESP_LOGW(TAG, "Timezone not applied automatically, applying manual offset");
        
        // Determine if we're in DST (roughly March to October)
        bool is_dst = (timeinfo.tm_mon >= 2 && timeinfo.tm_mon <= 9); // March=2, October=9
        int german_offset = is_dst ? 2 : 1; // CEST=UTC+2, CET=UTC+1
        
        // Apply German timezone offset
        now += german_offset * 3600; // Add hours in seconds
        localtime_r(&now, &timeinfo);
        
        ESP_LOGI(TAG, "Applied manual German offset: +%d hours (%s)", 
                 german_offset, is_dst ? "CEST" : "CET");
    }
    
    // Debug: Log current time being returned
    ESP_LOGD(TAG, "getCurrentLocalTime returning: %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    return true;
}
