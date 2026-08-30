#include "util/button_manager.h"
#include "build_config.h"
#include "config/pins.h"
#include "config/config_manager.h"
#include <driver/rtc_io.h>
#include <esp_system.h>
#include <Preferences.h>

static const char* TAG = "BUTTON";

// Synthetic button mode injected by long-press detection (-1 = none)
static int8_t syntheticButtonMode = -1;

// Volatile flag set by ISR when a button is pressed while device is awake.
// -1 = no press, 0/1/2 = display mode.
static volatile int8_t isrButtonPressed = -1;

// Timestamp when interrupts were attached — used for debounce
static unsigned long isrAttachTime = 0;

void ButtonManager::setSyntheticButtonMode(int8_t mode) {
    syntheticButtonMode = mode;
    ESP_LOGI(TAG, "Synthetic button mode set: %d", mode);
}

void ButtonManager::setWakupableButtons() {
    if (HAS_BUTTON) {
        ESP_LOGI(TAG, "Initializing button manager...");

        // Configure button pins as input with internal pull-up
        pinMode(Pins::BUTTON_HALF_AND_HALF, INPUT_PULLUP);
        pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
        pinMode(Pins::BUTTON_DEPARTURE_ONLY, INPUT_PULLUP);

        ESP_LOGI(TAG, "Button pins configured: GPIO %d, %d, %d",
                 Pins::BUTTON_HALF_AND_HALF,
                 Pins::BUTTON_WEATHER_ONLY, Pins::BUTTON_DEPARTURE_ONLY);

        // DIAGNOSTIC: Check if GPIOs support RTC (required for EXT1 wakeup)
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_HALF_AND_HALF)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_HALF_AND_HALF);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_HALF_AND_HALF);
        }
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_WEATHER_ONLY)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_WEATHER_ONLY);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_WEATHER_ONLY);
        }
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_DEPARTURE_ONLY)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_DEPARTURE_ONLY);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_DEPARTURE_ONLY);
        }
    } else {
        ESP_LOGI(TAG, "Button manager not available");
    }
}

int8_t ButtonManager::getWakeupButtonMode() {
    if (HAS_BUTTON) {
        if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
            return -1;
        }
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask == 0) return -1;

        if (wakeup_pin_mask & (1ULL << Pins::BUTTON_HALF_AND_HALF)) {
            ESP_LOGI(TAG, "Woken by BUTTON_HALF_AND_HALF");
            return DISPLAY_MODE_HALF_AND_HALF;
        } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_WEATHER_ONLY)) {
            ESP_LOGI(TAG, "Woken by BUTTON_WEATHER_ONLY");
            return DISPLAY_MODE_WEATHER_ONLY;
        } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_DEPARTURE_ONLY)) {
            ESP_LOGI(TAG, "Woken by BUTTON_DEPARTURE_ONLY");
            return DISPLAY_MODE_TRANSPORT_ONLY;
        }
    }
    return -1;
}

void ButtonManager::enableButtonWakeup() {
    if (HAS_BUTTON) {
        uint64_t button_mask = getButtonMask();
        esp_err_t result = esp_sleep_enable_ext1_wakeup(button_mask, ESP_EXT1_WAKEUP_ANY_LOW);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "✓ EXT1 wakeup enabled (mask: 0x%llx)", button_mask);
        } else {
            ESP_LOGE(TAG, "✗ EXT1 wakeup failed: %s", esp_err_to_name(result));
        }
    }
}

uint64_t ButtonManager::getButtonMask() {
    if (HAS_BUTTON) {
        return (1ULL << Pins::BUTTON_HALF_AND_HALF) |
               (1ULL << Pins::BUTTON_WEATHER_ONLY) |
               (1ULL << Pins::BUTTON_DEPARTURE_ONLY);
    }
    return 0;
}

// =============================================================================
// handleWakeupMode() — seconds-based temporary mode
// =============================================================================
void ButtonManager::handleWakeupMode() {
    if (!HAS_BUTTON) return;

    RTCConfigData& config = ConfigManager::getConfig();
    time_t currentTime;
    time(&currentTime);

    // Determine if a new button press occurred (from EXT1, synthetic mode, or NVS pending)
    int8_t buttonMode = syntheticButtonMode;
    bool dayAlreadySet = false; // true if awake-press already set selectedForecastDay
    if (buttonMode >= 0) {
        ESP_LOGI(TAG, "Consuming synthetic button mode: %d", buttonMode);
        syntheticButtonMode = -1;
    } else {
        buttonMode = getWakeupButtonMode();
    }

    // Check for pending temp mode saved to NVS by checkAndRestartIfButtonPressed()
    if (buttonMode < 0) {
        Preferences prefs;
        if (prefs.begin("mystation", false)) {
            if (prefs.getBool("pendingTemp", false)) {
                buttonMode = (int8_t)prefs.getUChar("pendingTempMode", 0xFF);
                // Restore selectedForecastDay if persisted (day browsing via awake-press)
                config.selectedForecastDay = prefs.getChar("pendingDay", 0);
                dayAlreadySet = true; // awake-press already computed the day
                // Clear the pending flags
                prefs.putBool("pendingTemp", false);
                prefs.remove("pendingTempMode");
                prefs.remove("pendingDay");
                ESP_LOGI(TAG, "Consumed pending temp mode from NVS: %d, day: %d", buttonMode, config.selectedForecastDay);
                if (buttonMode == (int8_t)0xFF) buttonMode = -1;
            }
            prefs.end();
        }
    }

    if (buttonMode >= 0) {
        // New button press — activate/reset temp mode
        // Check if we're in weather-only mode for day browsing
        if (config.displayMode == DISPLAY_MODE_WEATHER_ONLY) {
            if (!dayAlreadySet) {
                // Day browsing: reinterpret button presses (EXT1 / synthetic path)
                if (buttonMode == DISPLAY_MODE_HALF_AND_HALF) {
                    // Button 1: reset to today
                    config.selectedForecastDay = 0;
                    ESP_LOGI(TAG, "Day browse: reset to today (day 0)");
                } else if (buttonMode == DISPLAY_MODE_WEATHER_ONLY) {
                    // Button 2: forward (+1 day, circular 1→2→...→max-1→1, skip day 0)
                    int8_t maxDays = config.availableForecastDays > 1 ? config.availableForecastDays : 7;
                    int8_t next = config.selectedForecastDay + 1;
                    config.selectedForecastDay = (next >= maxDays) ? 1 : next;
                    ESP_LOGI(TAG, "Day browse: forward to day %d (max %d)", config.selectedForecastDay, maxDays);
                } else if (buttonMode == DISPLAY_MODE_TRANSPORT_ONLY) {
                    // Button 3: backward (-1 day, circular max-1→...→2→1→max-1, skip day 0)
                    int8_t maxDays = config.availableForecastDays > 1 ? config.availableForecastDays : 7;
                    int8_t prev = config.selectedForecastDay - 1;
                    config.selectedForecastDay = (prev < 1) ? (maxDays - 1) : prev;
                    ESP_LOGI(TAG, "Day browse: backward to day %d (max %d)", config.selectedForecastDay, maxDays);
                }
            }
            // Stay in weather-only mode with temp mode active
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
            config.temporaryModeActivationTime = (uint32_t)currentTime;
        } else {
            // Normal mode switching
            ESP_LOGI(TAG, "Button press! Activating temp mode: %d", buttonMode);
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = (uint8_t)buttonMode;
            config.temporaryModeActivationTime = (uint32_t)currentTime;
        }

    } else if (config.inTemporaryMode) {
        // No new button press — check if awake-press needs stamping or if expired

        if (config.temporaryModeActivationTime == 0) {
            // ISR set inTemporaryMode during awake cycle but couldn't stamp time.
            // Stamp it now (first boot after esp_restart).
            config.temporaryModeActivationTime = (uint32_t)currentTime;
            ESP_LOGI(TAG, "Awake-press boot — stamped activation time");
        }

        int elapsed = (int)(currentTime - config.temporaryModeActivationTime);
        ESP_LOGI(TAG, "Temp mode elapsed: %d/%d s", elapsed, TEMP_MODE_ACTIVE_DURATION);

        if (elapsed >= (int)TEMP_MODE_ACTIVE_DURATION) {
            config.inTemporaryMode = false;
            config.temporaryDisplayMode = 0xFF;
            config.temporaryModeActivationTime = 0;
            config.selectedForecastDay = 0;
            ESP_LOGI(TAG, "Temp mode expired — reverting to configured mode");
        }
    }
}

// =============================================================================
// GPIO ISR handlers — IRAM-safe, minimal work.
// Only the FIRST press is recorded; subsequent triggers are ignored until consumed.
// =============================================================================
static void IRAM_ATTR isrButton1() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_HALF_AND_HALF; }
static void IRAM_ATTR isrButton2() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_WEATHER_ONLY; }
static void IRAM_ATTR isrButton3() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_TRANSPORT_ONLY; }

void ButtonManager::attachRunningInterrupts() {
    if (!HAS_BUTTON) return;

    isrButtonPressed = -1;

    // De-init RTC GPIO for button pins — after deep sleep wakeup, pins may still
    // be in RTC mode which prevents normal GPIO interrupts from working.
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_HALF_AND_HALF);
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_WEATHER_ONLY);
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_DEPARTURE_ONLY);

    // Re-configure as normal GPIO with pull-up
    pinMode(Pins::BUTTON_HALF_AND_HALF, INPUT_PULLUP);
    pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
    pinMode(Pins::BUTTON_DEPARTURE_ONLY, INPUT_PULLUP);
    delay(10); // Allow pull-ups to stabilize

    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_HALF_AND_HALF), isrButton1, FALLING);
    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_WEATHER_ONLY),  isrButton2, FALLING);
    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_DEPARTURE_ONLY), isrButton3, FALLING);

    // Record attach time for debounce — ignore ISR triggers in the first 500ms
    isrAttachTime = millis();

    // Discard any ISR that fired during attachment (pin was already LOW)
    isrButtonPressed = -1;

    ESP_LOGI(TAG, "Running-mode interrupts attached (GPIO %d, %d, %d)",
             Pins::BUTTON_HALF_AND_HALF, Pins::BUTTON_WEATHER_ONLY, Pins::BUTTON_DEPARTURE_ONLY);
}

bool ButtonManager::checkAndRestartIfButtonPressed() {
    if (!HAS_BUTTON) return false;

    int8_t mode = isrButtonPressed;
    if (mode < 0) return false;

    // Debounce: ignore if triggered within 500ms of attaching interrupts
    if ((millis() - isrAttachTime) < 500) {
        isrButtonPressed = -1;
        return false;
    }

    ESP_LOGI(TAG, "Button pressed while awake (mode %d) — activating temp mode, restarting", mode);

    // In weather-only mode, reinterpret button as day browsing before persisting
    RTCConfigData& config = ConfigManager::getConfig();
    uint8_t persistMode = (uint8_t)mode;
    if (config.displayMode == DISPLAY_MODE_WEATHER_ONLY) {
        // Day browsing: update selectedForecastDay and keep weather-only mode
        int8_t maxDays = config.availableForecastDays > 1 ? config.availableForecastDays : 7;
        if (mode == DISPLAY_MODE_HALF_AND_HALF) {
            config.selectedForecastDay = 0;
        } else if (mode == DISPLAY_MODE_WEATHER_ONLY) {
            int8_t next = config.selectedForecastDay + 1;
            config.selectedForecastDay = (next >= maxDays) ? 1 : next;
        } else if (mode == DISPLAY_MODE_TRANSPORT_ONLY) {
            int8_t prev = config.selectedForecastDay - 1;
            config.selectedForecastDay = (prev < 1) ? (maxDays - 1) : prev;
        }
        persistMode = DISPLAY_MODE_WEATHER_ONLY;
        ESP_LOGI(TAG, "Day browse (awake): selected day %d (max %d)", config.selectedForecastDay, maxDays);
    }

    // Persist pending temp mode to NVS so it survives esp_restart()
    // (RTC_DATA_ATTR is lost on software reset, only preserved across deep sleep)
    Preferences prefs;
    if (prefs.begin("mystation", false)) {
        prefs.putBool("pendingTemp", true);
        prefs.putUChar("pendingTempMode", persistMode);
        prefs.putChar("pendingDay", config.selectedForecastDay);
        prefs.end();
        ESP_LOGI(TAG, "Saved pending temp mode %d, day %d to NVS", persistMode, config.selectedForecastDay);
    } else {
        ESP_LOGE(TAG, "Failed to save pending temp mode to NVS");
    }

    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_HALF_AND_HALF));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_WEATHER_ONLY));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_DEPARTURE_ONLY));

    esp_restart();
    return true; // unreachable
}
