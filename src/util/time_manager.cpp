#include "util/time_manager.h"
#include <time.h>
#include <esp_sntp.h>

static const char* TAG = "TIME_MGR";

// ============================================================================
// RTC-persisted state for NTP sync scheduling
// ============================================================================

// Epoch timestamp of last successful NTP sync (survives deep sleep, lost on power loss)
RTC_DATA_ATTR static uint32_t lastNtpSyncEpoch = 0;

// ============================================================================
// Public API — Time Display & Query
// ============================================================================

String TimeManager::getGermanDateTimeString() {
    tm timeinfo;
    if (!getCurrentLocalTime(timeinfo)) {
        ESP_LOGW(TAG, "Failed to get local time for German date/time string");
        return String("--:-- --.--.---- -------");
    }

    static const char* dayNames[] = {
        "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
    };
    char buf[40];
    int wday = timeinfo.tm_wday;
    if (wday < 0 || wday > 6) wday = 0;

    snprintf(buf, sizeof(buf), "%02d:%02d %02d.%02d.%04d %s",
             timeinfo.tm_hour, timeinfo.tm_min,
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
             dayNames[wday]);
    return String(buf);
}

void TimeManager::printCurrentTime() {
    time_t now = time(nullptr);
    tm* timeinfo = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

bool TimeManager::isTimeSet() {
    time_t now = time(nullptr);
    return now > 8 * 3600 * 2; // Check if year > 1971
}

bool TimeManager::getCurrentLocalTime(tm& timeinfo) {
    if (!isTimeSet()) {
        ESP_LOGW(TAG, "Time not set, cannot get current local time");
        return false;
    }

    // Always ensure timezone is set before getting local time
    // This is necessary because TZ environment may not persist across deep sleep
    // German timezone: CET-1CEST,M3.5.0,M10.5.0/3 means:
    // - CET (Central European Time) is UTC+1
    // - CEST (Central European Summer Time) is UTC+2
    // M3.5.0 = DST starts on month 3 (March), week 5 (last), day 0 (Sunday) at 02:00
    // M10.5.0/3 = DST ends on month 10 (October), week 5 (last), day 0 (Sunday) at 03:00
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);

    ESP_LOGD(TAG, "getCurrentLocalTime returning: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    return true;
}

// ============================================================================
// NTP Synchronization — once per day to correct RTC drift
//
// ESP32's internal 150kHz RC oscillator drifts ~5-15 seconds/day.
// We sync once per 24h using SNTP_SYNC_MODE_IMMED which forces an instant
// time jump (instead of gradual adjtime() which is useless for deep-sleep
// devices that are only awake for seconds).
// ============================================================================

bool TimeManager::needsPeriodicSync() {
    // Never synced — must sync
    if (lastNtpSyncEpoch == 0) {
        ESP_LOGI(TAG, "No previous NTP sync recorded - sync needed");
        return true;
    }

    // Get current RTC time
    time_t now = time(nullptr);
    if (now < 8 * 3600 * 2) {
        // Time not valid (shouldn't happen after first successful sync)
        ESP_LOGI(TAG, "RTC time invalid - sync needed");
        return true;
    }

    // Check if sync interval has elapsed
    uint32_t elapsed = (uint32_t)now - lastNtpSyncEpoch;
    bool needsSync = elapsed >= SYNC_INTERVAL_SECONDS;

    if (needsSync) {
        ESP_LOGI(TAG, "Last NTP sync %u seconds ago (%u hours) - sync needed",
                 elapsed, elapsed / 3600);
    } else {
        ESP_LOGD(TAG, "Last NTP sync %u seconds ago (%u hours) - RTC time OK",
                 elapsed, elapsed / 3600);
    }

    return needsSync;
}

// Volatile flag set by SNTP callback when time is synchronized
static volatile bool ntpSyncDone = false;

static void ntpSyncCallback(struct timeval* tv) {
    ntpSyncDone = true;
}

bool TimeManager::setupNTPTimeWithRetry(int maxRetries) {
    ESP_LOGI(TAG, "Starting NTP time sync (IMMED mode, timeout: %d ms)", NTP_TIMEOUT_MS);

    // Use IMMED mode — forces instant time correction instead of gradual adjtime().
    // Without this, drifts < 35 minutes are "smoothed" over time, which is useless
    // for a deep-sleep device that's only awake for seconds.
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);

    // Register callback — fires when SNTP response is received and time is applied
    sntp_set_time_sync_notification_cb(ntpSyncCallback);

    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        ESP_LOGI(TAG, "NTP sync attempt %d/%d", attempt, maxRetries);

        ntpSyncDone = false;

        // Configure timezone and start SNTP client
        configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");

        // Wait for the callback to fire (NTP response received and applied)
        int waitedMs = 0;
        while (!ntpSyncDone && waitedMs < NTP_TIMEOUT_MS) {
            delay(100);
            waitedMs += 100;
        }

        if (ntpSyncDone) {
            time_t now = time(nullptr);
            lastNtpSyncEpoch = (uint32_t)now;

            tm timeinfo;
            localtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "NTP sync successful (attempt %d, %d ms) - %04d-%02d-%02d %02d:%02d:%02d",
                     attempt, waitedMs,
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

            sntp_stop();
            return true;
        }

        ESP_LOGW(TAG, "NTP sync attempt %d timed out after %d ms", attempt, NTP_TIMEOUT_MS);
        sntp_stop();

        if (attempt < maxRetries) {
            delay(1000);
        }
    }

    ESP_LOGE(TAG, "All NTP sync attempts failed after %d tries", maxRetries);
    return false;
}
