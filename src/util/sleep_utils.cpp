#include "util/time_manager.h"
#include "util/sleep_utils.h"
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

// Enhanced deep sleep function with multiple wakeup options
void enterDeepSleep(uint64_t sleepTimeSeconds) {
    if (sleepTimeSeconds <= 0) {
        ESP_LOGE(TAG, "Invalid sleep time: %llu, not entering deep sleep!", sleepTimeSeconds);
        return;
    }

    // Setup time synchronization after WiFi connection
    if (!TimeManager::isTimeSet()) {
        ESP_LOGI(TAG, "Time not set, synchronizing with NTP...");
        TimeManager::setupNTPTime();
    }

    ESP_LOGI(TAG, "Entering deep sleep for %u seconds (%u minutes)",
             sleepTimeSeconds, sleepTimeSeconds / 60);

    // Configure timer wakeup
    esp_sleep_enable_timer_wakeup(sleepTimeSeconds * 1000000ULL); // Convert seconds to microseconds

    // Optional: Configure GPIO wakeup (e.g., for user button)
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_9, 0); // Wake on low signal on GPIO 9

    // can cause issues after deep sleep, especially if the serial monitor is not actively reading data.
    // It can cause the ESP32 to block on the Serial.flush() call if the serial port is not properly re-established after deep sleep.
    // Flush serial output
    // Serial.flush();

    // Enter deep sleep
    esp_deep_sleep_start();
}
