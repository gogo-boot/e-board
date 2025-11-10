#pragma once
#include <Arduino.h>
#include <esp_sleep.h>

// Button manager for temporary display mode switching (ESP32-S3 only)
class ButtonManager {
public:
    // Timeout for temporary mode (2 minutes in seconds)
    static constexpr uint32_t TEMPORARY_MODE_TIMEOUT = 120;

    // Debounce delay in milliseconds
    static constexpr uint32_t DEBOUNCE_DELAY_MS = 50;

    // Initialize button GPIO pins
    static void init();

    // Check if any button was pressed (reads current state with debouncing)
    static bool isButtonPressed(int buttonPin);

    // Check which button caused EXT1 wakeup from deep sleep
    // Returns the display mode corresponding to the button, or -1 if no button
    static int8_t getWakeupButtonMode();

    // Enable EXT1 wakeup for all buttons before entering deep sleep
    static void enableButtonWakeup();

    // Check if wakeup was caused by button press
    static bool wasWokenByButton();

private:
    // Get GPIO mask for all three buttons
    static uint64_t getButtonMask();
};


