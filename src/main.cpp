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

// Temporary global config for dynamic data (API results)
// This is only used for stopNames, stopIds, and stopDistances from API calls
MyStationConfig g_stationConfig;

float g_lat = 0.0, g_lon = 0.0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect

  esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  
  // Determine device mode based on saved configuration
  if (DeviceModeManager::hasValidConfiguration()) {
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
