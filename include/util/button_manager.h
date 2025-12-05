#pragma once
#include <Arduino.h>

// Button manager for temporary display mode switching (ESP32-S3 only)
class ButtonManager {
public:
    // Duration for temporary mode in active hours (2 minutes in seconds)
    static constexpr uint32_t TEMP_MODE_ACTIVE_DURATION = 120;

    // Debounce delay in milliseconds
    static constexpr uint32_t DEBOUNCE_DELAY_MS = 50;

    // Initialize button GPIO pins
    static void setWakupableButtons();

    // Check which button caused EXT1 wakeup from deep sleep
    // Returns the display mode corresponding to the button, or -1 if no button
    static int8_t getWakeupButtonMode();

    // Enable EXT1 wakeup for all buttons before entering deep sleep
    static void enableButtonWakeup();

    // Handle button wakeup and set temporary display mode if needed
    // Should be called early in boot process
    static void handleWakeupMode();

private:
    // Get GPIO mask for all three buttons
    static uint64_t getButtonMask();
};


