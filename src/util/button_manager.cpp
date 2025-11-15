#include "util/button_manager.h"
#include "config/pins.h"
#include "config/config_manager.h"
#include <esp_log.h>
#include <driver/rtc_io.h>

static const char* TAG = "BUTTON";

void ButtonManager::init() {
#ifdef BOARD_ESP32_S3
    ESP_LOGI(TAG, "Initializing button manager...");

    // Configure button pins as input with internal pull-up
    pinMode(Pins::BUTTON_HALF_AND_HALF, INPUT_PULLUP);
    pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
    pinMode(Pins::BUTTON_DEPARTURE_ONLY, INPUT_PULLUP);

    ESP_LOGI(TAG, "Button pins configured: GPIO %d, %d, %d",
             Pins::BUTTON_HALF_AND_HALF,
             Pins::BUTTON_WEATHER_ONLY, Pins::BUTTON_DEPARTURE_ONLY);

    // DIAGNOSTIC: Check if GPIOs support RTC (required for EXT1 wakeup)
    ESP_LOGI(TAG, "Checking RTC GPIO support...");

    // Check GPIO 2
    if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_HALF_AND_HALF)) {
        ESP_LOGI(TAG, "✓ GPIO %d supports RTC (EXT1 wakeup)", Pins::BUTTON_HALF_AND_HALF);
    } else {
        ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC - cannot wake from deep sleep!",
                 Pins::BUTTON_HALF_AND_HALF);
    }

    // Check GPIO 3
    if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_WEATHER_ONLY)) {
        ESP_LOGI(TAG, "✓ GPIO %d supports RTC (EXT1 wakeup)", Pins::BUTTON_WEATHER_ONLY);
    } else {
        ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC - cannot wake from deep sleep!",
                 Pins::BUTTON_WEATHER_ONLY);
    }

    // Check GPIO 5 (or 4)
    if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_DEPARTURE_ONLY)) {
        ESP_LOGI(TAG, "✓ GPIO %d supports RTC (EXT1 wakeup)", Pins::BUTTON_DEPARTURE_ONLY);
    } else {
        ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC - cannot wake from deep sleep!",
                 Pins::BUTTON_DEPARTURE_ONLY);
    }

    // DIAGNOSTIC: Check current button states
    delay(100); // Allow pull-ups to stabilize
    ESP_LOGI(TAG, "Current button states:");
    ESP_LOGI(TAG, "  GPIO %d: %s", Pins::BUTTON_HALF_AND_HALF,
             digitalRead(Pins::BUTTON_HALF_AND_HALF) == HIGH ? "HIGH (not pressed)" : "LOW (PRESSED!)");
    ESP_LOGI(TAG, "  GPIO %d: %s", Pins::BUTTON_WEATHER_ONLY,
             digitalRead(Pins::BUTTON_WEATHER_ONLY) == HIGH ? "HIGH (not pressed)" : "LOW (PRESSED!)");
    ESP_LOGI(TAG, "  GPIO %d: %s", Pins::BUTTON_DEPARTURE_ONLY,
             digitalRead(Pins::BUTTON_DEPARTURE_ONLY) == HIGH ? "HIGH (not pressed)" : "LOW (PRESSED!)");

    // Print valid RTC GPIOs for ESP32-S3
    ESP_LOGI(TAG, "ESP32-S3 valid RTC GPIOs for EXT1 wakeup:");
    ESP_LOGI(TAG, "  GPIO 0-21 (most common for buttons: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");

#else
    ESP_LOGI(TAG, "Button manager not available (ESP32-S3 only)");
#endif
}

int8_t ButtonManager::getWakeupButtonMode() {
#ifdef BOARD_ESP32_S3
    // Check if wakeup was caused by EXT1 (button press)
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        return -1;
    }

    // Get which GPIO caused the wakeup
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

    if (wakeup_pin_mask == 0) {
        ESP_LOGW(TAG, "EXT1 wakeup but no pin detected");
        return -1;
    }

    // Check which button was pressed
    if (wakeup_pin_mask & (1ULL << Pins::BUTTON_HALF_AND_HALF)) {
        ESP_LOGI(TAG, "Woken by BUTTON_HALF_AND_HALF (GPIO %d)", Pins::BUTTON_HALF_AND_HALF);
        return DISPLAY_MODE_HALF_AND_HALF;
    } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_WEATHER_ONLY)) {
        ESP_LOGI(TAG, "Woken by BUTTON_WEATHER_ONLY (GPIO %d)", Pins::BUTTON_WEATHER_ONLY);
        return DISPLAY_MODE_WEATHER_ONLY;
    } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_DEPARTURE_ONLY)) {
        ESP_LOGI(TAG, "Woken by BUTTON_DEPARTURE_ONLY (GPIO %d)", Pins::BUTTON_DEPARTURE_ONLY);
        return DISPLAY_MODE_DEPARTURE_ONLY;
    }

    ESP_LOGW(TAG, "EXT1 wakeup from unknown button: 0x%llx", wakeup_pin_mask);
#endif
    return -1;
}

void ButtonManager::enableButtonWakeup() {
#ifdef BOARD_ESP32_S3
    uint64_t button_mask = getButtonMask();

    ESP_LOGI(TAG, "Enabling EXT1 wakeup for buttons...");
    ESP_LOGI(TAG, "  Button mask: 0x%llx", button_mask);
    ESP_LOGI(TAG, "  GPIO %d bit: %d", Pins::BUTTON_HALF_AND_HALF,
             (button_mask & (1ULL << Pins::BUTTON_HALF_AND_HALF)) ? 1 : 0);
    ESP_LOGI(TAG, "  GPIO %d bit: %d", Pins::BUTTON_WEATHER_ONLY,
             (button_mask & (1ULL << Pins::BUTTON_WEATHER_ONLY)) ? 1 : 0);
    ESP_LOGI(TAG, "  GPIO %d bit: %d", Pins::BUTTON_DEPARTURE_ONLY,
             (button_mask & (1ULL << Pins::BUTTON_DEPARTURE_ONLY)) ? 1 : 0);

    // Enable EXT1 wakeup on ANY_LOW (any button press will wake the device)
    esp_err_t result = esp_sleep_enable_ext1_wakeup(button_mask, ESP_EXT1_WAKEUP_ANY_LOW);

    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✓ EXT1 wakeup enabled successfully (mask: 0x%llx)", button_mask);
    } else {
        ESP_LOGE(TAG, "✗ Failed to enable EXT1 wakeup! Error: 0x%x (%s)",
                 result, esp_err_to_name(result));
        ESP_LOGE(TAG, "  This usually means one or more GPIOs don't support RTC!");
    }
#endif
}

uint64_t ButtonManager::getButtonMask() {
#ifdef BOARD_ESP32_S3
    // Create mask for all three button GPIOs
    return (1ULL << Pins::BUTTON_HALF_AND_HALF) |
        (1ULL << Pins::BUTTON_WEATHER_ONLY) |
        (1ULL << Pins::BUTTON_DEPARTURE_ONLY);
#else
    return 0;
#endif
}

void ButtonManager::handleWakeupMode() {
#ifdef BOARD_ESP32_S3
    // Check if device was woken by button press (temporary display mode)
    int8_t buttonMode = getWakeupButtonMode();
    if (buttonMode >= 0) {
        ESP_LOGI(TAG, "Woken by button press! Temporary display mode: %d", buttonMode);

        // Store temporary mode information in RTC memory
        RTCConfigData& config = ConfigManager::getConfig();
        config.inTemporaryMode = true;
        config.temporaryDisplayMode = buttonMode;
    }
#endif
}


