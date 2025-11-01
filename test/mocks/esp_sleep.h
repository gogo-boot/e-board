#pragma once

#include <cstdint>

// Mock ESP32 sleep functions for native testing
typedef enum {
    ESP_SLEEP_WAKEUP_UNDEFINED,
    ESP_SLEEP_WAKEUP_EXT0,
    ESP_SLEEP_WAKEUP_EXT1,
    ESP_SLEEP_WAKEUP_TIMER,
    ESP_SLEEP_WAKEUP_TOUCHPAD,
    ESP_SLEEP_WAKEUP_ULP
} esp_sleep_wakeup_cause_t;

inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() {
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

inline void esp_deep_sleep_start() {
    // No-op for mock
}

inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
    // No-op for mock
}

