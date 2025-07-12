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
#include <esp_log.h>
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "util/device_mode_manager.h"

#include <SPI.h>
//EPD
#include "config/pins.h"

// GxEPD2 display library includes for GDEY075T7 (800x480)
  //IO settings
//SCLK--GPIO18
//MOSI--GPIO23
#define isEPD_W21_BUSY digitalRead(Pins::EPD_BUSY)  //BUSY
#define EPD_W21_RST_0 digitalWrite(Pins::EPD_RES,LOW)  //RES
#define EPD_W21_RST_1 digitalWrite(Pins::EPD_RES,HIGH)
#define EPD_W21_DC_0  digitalWrite(Pins::EPD_DC,LOW) //DC
#define EPD_W21_DC_1  digitalWrite(Pins::EPD_DC,HIGH)
#define EPD_W21_CS_0 digitalWrite(Pins::EPD_CS,LOW) //CS
#define EPD_W21_CS_1 digitalWrite(Pins::EPD_CS,HIGH)

#define EPD_WIDTH   800 
#define EPD_HEIGHT  480
#define EPD_ARRAY  EPD_WIDTH*EPD_HEIGHT/8  
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>  // Specific driver for GDEY075T7

// Create display instance for GDEY075T7 (800x480 resolution)
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(GxEPD2_750_GDEY075T7(Pins::EPD_CS, Pins::EPD_DC, Pins::EPD_RES, Pins::EPD_BUSY));

static const char* TAG = "MAIN";

// --- Globals (shared across modules) ---
WebServer server(80);
RTC_DATA_ATTR unsigned long loopCount = 0;
RTC_DATA_ATTR bool hasValidConfig = false; // Flag to track if valid config exists

// This Struct is only for showing on configureation web interface
// It is used to hold dynamic data like stopNames, stopIds, and stopDistances from API calls
// This will not be used to store configuration data in NVS
ConfigOption g_webConfigPageData;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect, only for local debugging, todo remove in production or activate by flag

  // esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");

  // Initialize pins for e-paper display
  pinMode(Pins::EPD_BUSY, INPUT);  //BUSY
  pinMode(Pins::EPD_RES, OUTPUT); //RES 
  pinMode(Pins::EPD_DC, OUTPUT); //DC   
  pinMode(Pins::EPD_CS, OUTPUT); //CS   

  // TODO: Add any additional display initialization code here

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
