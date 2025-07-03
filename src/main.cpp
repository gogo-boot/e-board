/*
 * MyStation E-Board - ESP32-C3 Public Transport Departure Board
 * 
 * Boot Process Flow:
 * 1. System starts and calls DeviceModeManager::hasValidConfiguration()
 * 2. If no valid config -> DeviceModeManager::runConfigurationMode()
 *    - Creates WiFi hotspot for setup
 *    - Starts web server for user configuration
 *    - Stays awake to handle configuration
 * 3. If valid config exists -> DeviceModeManager::runOperationalMode()
 *    - Connects to saved WiFi
 *    - Fetches transport and weather data
 *    - Updates display and enters deep sleep
 * 
 * Configuration persists in NVS (Non-Volatile Storage) across:
 * - Power loss/battery changes
 * - Firmware updates
 * - Manual resets
 */

#define ARDUINOJSON_DECODE_NESTING_LIMIT 200
#include <Arduino.h>
#include <WebServer.h>
#include "esp_log.h"
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "util/device_mode_manager.h"

static const char* TAG = "MAIN";

// --- Globals (shared across modules) ---
WebServer server(80);
RTC_DATA_ATTR unsigned long loopCount = 0;
RTC_DATA_ATTR bool hasValidConfig = false; // Flag to track if valid config exists

// This Struct is only for showing on configureation web interface
// It is used to hold dynamic data like stopNames, stopIds, and stopDistances from API calls
// This will not be used to store configuration data in NVS
ConfigOption g_configOption;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect, only for local debugging, todo remove in production or activate by flag

  // esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  
  // Determine device mode based on saved configuration
  if ( hasValidConfig || DeviceModeManager::hasValidConfiguration(hasValidConfig)) {
    DeviceModeManager::runOperationalMode();
  } else {
    DeviceModeManager::runConfigurationMode();
  }
}

void loop() {
  // Only handle web server in config mode
  if (ConfigManager::isConfigMode()) {
    server.handleClient();
    delay(10); // Small delay to prevent watchdog issues
  } else {
    // Normal operation happens in setup() and then device goes to sleep
    // This should never be reached in normal operation
    ESP_LOGW(TAG, "Unexpected: loop() called in normal operation mode");
    delay(5000);
  }
}
