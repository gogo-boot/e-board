/*
 * MyStation E-Board - ESP32 Public Transport Departure Board
 *
 * Boot Process Flow:
 * 1. System Initialization (SystemInit)
 *    - Hardware setup, logging, diagnostics
 *    - Factory reset check
 *    - Battery and button initialization
 *    - Load configuration from NVS
 *
 * 2. OTA Update Check (OTAManager)
 *    - Check if scheduled OTA time
 *    - Download and apply updates if available
 *
 * 3. Button Wakeup Handling (ButtonManager)
 *    - Detect button press wakeup
 *    - Set temporary display mode
 *
 * 4. Boot Flow Execution (BootFlowManager)
 *    - PHASE 1: WiFi Setup (if no WiFi configured)
 *    - PHASE 2: App Setup (if WiFi configured, app settings needed)
 *    - PHASE 3: Operational Mode (fully configured)
 *
 * Configuration persists in NVS (Non-Volatile Storage) across:
 * - Power loss/battery changes
 * - Firmware updates
 * - Manual resets
 */

#define ARDUINOJSON_DECODE_NESTING_LIMIT 1000

#include <Arduino.h>

#include "activity/activity_manager.h"
// Configuration
#include "config/config_manager.h"
#include "config/pins.h"
#include "global_instances.h"

static const char* TAG = "MAIN";

// =============================================================================
// Global Components (shared across modules)
// =============================================================================

// Web server for configuration mode
WebServer server(80);

// E-Paper display for GDEY075T7 (800x480)
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
    GxEPD2_750_GDEY075T7(Pins::EPD_CS, Pins::EPD_DC, Pins::EPD_RES, Pins::EPD_BUSY));

// U8g2 font renderer for UTF-8 support (German umlauts)
U8G2_FOR_ADAFRUIT_GFX u8g2;

// RTC memory for persistent state across deep sleep
RTC_DATA_ATTR unsigned long wakeupCount = 0;

// =============================================================================
// Main Entry Points
// =============================================================================

void setup() {
    wakeupCount++;

    // OnInit: System Initialization Phase which prepares for other phases
    ActivityManager::onInit();

    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_START) {
        ActivityManager::onStart();
    }
    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_RUNNING) {
        ActivityManager::onRunning();
    }
    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_STOP) {
        ActivityManager::onStop();
    }
    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_SHUTDOWN) {
        ActivityManager::onShutdown();
    }
    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_LOOP) {
        ESP_LOGI(TAG, "Starting loop");
    }
}

void loop() {
    // Only handle web server in config mode
    if (ActivityManager::getNextActivityLifecycle() == Lifecycle::ON_LOOP) {
        server.handleClient();
        delay(10); // Small delay to prevent watchdog issues
    } else {
        // Normal operation happens in setup() and then device goes to sleep
        // This should never be reached in normal operation
        ESP_LOGW(TAG, "Unexpected: loop() called in normal operation mode");
        delay(5000);
    }
}
