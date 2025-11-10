#include "util/time_manager.h"
#include "util/sleep_utils.h"
#include "util/button_manager.h"
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <time.h>

static const char* TAG = "SLEEP";

// Print wakeup reason after deep sleep
void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
        ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        ESP_LOGI(TAG, "Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        ESP_LOGI(TAG, "Wakeup caused by ULP program");
        break;
    default:
        ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d", wakeup_reason);
        break;
    }
}

// Deep sleep with both timer and button wakeup enabled (ESP32-S3 only)
void enterDeepSleep(uint64_t sleepTimeSeconds) {
    if (sleepTimeSeconds <= 0) {
        ESP_LOGE(TAG, "Invalid sleep time: %llu, not entering deep sleep!", sleepTimeSeconds);
        return;
    }

    // Setup time synchronization if needed
    if (!TimeManager::isTimeSet()) {
        ESP_LOGI(TAG, "Time not set, synchronizing with NTP...");
        TimeManager::setupNTPTime();
    }
    ESP_LOGI(TAG, "Entering deep sleep for %u seconds (%llu minutes)",
             sleepTimeSeconds, sleepTimeSeconds / 60);

    // Configure timer wakeup
    esp_sleep_enable_timer_wakeup(sleepTimeSeconds * 1000000ULL); // Convert seconds to microseconds

#ifdef BOARD_ESP32_S3
    // Enable button wakeup (EXT1)
    ButtonManager::enableButtonWakeup();
#endif

    // Enter deep sleep
    esp_deep_sleep_start();
}

