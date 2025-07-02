/*
 * MyStation E-Board - ESP32-C3 Public Transport Departure Board
 * 
 * Boot Process Flow:
 * 1. System starts and calls hasConfigInNVS()
 * 2. If no valid config -> runConfigurationMode()
 *    - Creates WiFi hotspot for setup
 *    - Starts web server for user configuration
 *    - Stays awake to handle configuration
 * 3. If valid config exists -> runOperationalMode()
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
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_log.h"
#include "secrets/google_secrets.h"
#include "secrets/rmv_secrets.h"
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include "api/rmv_api.h"
#include "api/google_api.h"
#include "api/dwd_weather_api.h"
#include "util/util.h"
#include "config/config_page.h"
#include "config/config_manager.h" // Add new config manager
#include "api/google_api.h" // Add this if getCityFromLatLon is declared here
#include "util/power.h"
#include "util/weather_print.h"
#include "util/sleep_utils.h"
#include <time.h>
#include "config/config_struct.h"
#include "config/pins.h"
#include <ESPmDNS.h>
#include "esp_sleep.h"

static const char* TAG = "MAIN";

// Function declarations
bool loadConfig(MyStationConfig &config);
bool hasConfigInNVS();
void runConfigurationMode();
void runOperationalMode();

// --- Globals ---
WebServer server(80);
RTC_DATA_ATTR bool inConfigMode = true; // RTC memory for fast access during deep sleep
RTC_DATA_ATTR unsigned long loopCount = 0;

float g_lat = 0.0, g_lon = 0.0;
MyStationConfig g_stationConfig;

// --- Modularized Setup Functions ---
void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    ESP_LOGD(TAG, "WiFi already connected: %s", WiFi.localIP().toString().c_str());
    return; // Already connected
  }
  
  ESP_LOGI(TAG, "WiFi disconnected, attempting to reconnect...");
  
  // Try to reconnect using saved credentials
  if (g_stationConfig.ssid.length() > 0) {
    ESP_LOGI(TAG, "Attempting to connect to saved SSID: %s", g_stationConfig.ssid.c_str());
    WiFi.begin(g_stationConfig.ssid.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 3) {
      delay(500);
      ESP_LOGD(TAG, "Connecting to WiFi... attempt %d", attempts + 1);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      ESP_LOGI(TAG, "WiFi reconnected successfully!");
      ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
      g_stationConfig.ipAddress = WiFi.localIP().toString();
    } else {
      ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
    }
  } else {
    ESP_LOGW(TAG, "No saved WiFi credentials available for reconnection");
  }
}

void setupWiFiStationMode(MyStationConfig &config) {
  // Station mode only - connect to saved WiFi without AP mode
  ESP_LOGI(TAG, "Connecting to WiFi in station mode...");
  
  if (config.ssid.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 3) { // 10 seconds timeout
      delay(500);
      ESP_LOGD(TAG, "Connecting to WiFi... attempt %d", attempts + 1);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      ESP_LOGI(TAG, "WiFi connected successfully!");
      ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
      config.ipAddress = WiFi.localIP().toString();
      
      // Start mDNS in station mode
      if (MDNS.begin("mystation")) {
        ESP_LOGI(TAG, "mDNS responder started: http://mystation.local");
      }
    } else {
      ESP_LOGW(TAG, "Failed to connect to WiFi in station mode");
    }
  } else {
    ESP_LOGW(TAG, "No saved WiFi credentials available");
  }
}

void setupWiFiAndMDNS(WiFiManager &wm, MyStationConfig &config) {
  ESP_LOGD(TAG, "Starting WiFiManager AP mode...");
  const char *menu[] = {"wifi"};
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
  String apName = Util::getUniqueSSID("MyStation");
  ESP_LOGD(TAG, "AP SSID: %s", apName.c_str());
  bool res = wm.autoConnect(apName.c_str());
  ESP_LOGD(TAG, "autoConnect() returned");
  if (!res) {
    ESP_LOGE(TAG, "Failed to connect");
    return;
  }
  ESP_LOGI(TAG, "WiFi connected!");
  config.ssid = wm.getWiFiSSID();
  if (MDNS.begin("mystation")) {
    ESP_LOGI(TAG, "mDNS responder started: http://mystation.local");
  } else {
    ESP_LOGW(TAG, "mDNS responder failed to start");
  }
  ESP_LOGI(TAG, "ESP32 IP address: %s", WiFi.localIP().toString().c_str());
  config.ipAddress = WiFi.localIP().toString();
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  ESP_LOGI(TAG, "Waiting for NTP time sync");
  time_t now = time(nullptr);
  int retry = 0;
  const int retry_count = 30;
  while (now < 8 * 3600 * 2 && retry < retry_count) { // year < 1971
    delay(500);
    ESP_LOGD(TAG, ".");
    now = time(nullptr);
    retry++;
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  ESP_LOGI(TAG, "NTP time set: %04d-%02d-%02d %02d:%02d:%02d",
    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void setupWebServer() {
  server.on("/", []() { handleConfigPage(server); });
  server.on("/save_config", HTTP_POST, []() { handleSaveConfig(server, inConfigMode ); });
  server.on("/api/stop", HTTP_GET, []() { handleStopAutocomplete(server); });
  server.begin();
  ESP_LOGI(TAG, "HTTP server started.");
}

// --- Mode Detection Function ---
bool hasConfigInNVS() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  
  // Load configuration from NVS
  bool configExists = configMgr.loadConfig(g_stationConfig);
  
  if (!configExists) {
    ESP_LOGI(TAG, "No configuration found in NVS");
    return false;
  }
  
  // Validate critical configuration fields
  bool hasValidConfig = (g_stationConfig.selectedStopId.length() > 0 && 
                        g_stationConfig.ssid.length() > 0 &&
                        g_stationConfig.latitude != 0.0 && 
                        g_stationConfig.longitude != 0.0);
  
  ESP_LOGI(TAG, "- SSID: %s", g_stationConfig.ssid.c_str());
  ESP_LOGI(TAG, "- Stop: %s (%s)", g_stationConfig.selectedStopName.c_str(), g_stationConfig.selectedStopId.c_str());
  ESP_LOGI(TAG, "- Location: %s (%f, %f)", g_stationConfig.cityName.c_str(), g_stationConfig.latitude, g_stationConfig.longitude);

  if (hasValidConfig) {
    ESP_LOGI(TAG, "Valid configuration found in NVS");
    return true;
  } else {
    ESP_LOGI(TAG, "Incomplete configuration in NVS, missing critical fields");
    return false;
  }
}

// --- Configuration Mode ---
void runConfigurationMode() {
  ESP_LOGI(TAG, "=== ENTERING CONFIGURATION MODE ===");
  
  ConfigManager& configMgr = ConfigManager::getInstance();
  
  // Set configuration mode flag
  inConfigMode = true;
  configMgr.saveConfigMode(true);
  
  // Initialize config with defaults
  g_stationConfig.latitude = 0.0;
  g_stationConfig.longitude = 0.0;
  g_stationConfig.ssid = WiFi.SSID(); // Use current SSID if available
  g_stationConfig.ipAddress = WiFi.localIP().toString();
  g_stationConfig.ssid = "";
  g_stationConfig.cityName = "";
  g_stationConfig.oepnvFilters = {"RE", "S-Bahn", "Bus"};

  // Initialize filesystem
  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
    while (true) {
      delay(1000);
    }
  }

  // Setup WiFi with access point for configuration
  WiFiManager wm;
  setupWiFiAndMDNS(wm, g_stationConfig);
  
  // Setup time synchronization
  setupTime();
  
  // Load any existing partial configuration
  configMgr.loadConfig(g_stationConfig);
  
  // Get location if not already saved
  if (g_stationConfig.latitude == 0.0 && g_stationConfig.longitude == 0.0) {
    getLocationFromGoogle(g_lat, g_lon);
    getCityFromLatLon(g_lat, g_lon);
    ESP_LOGI(TAG, "City set in setup: %s", g_stationConfig.cityName.c_str());
  } else {
    // Use saved coordinates
    g_lat = g_stationConfig.latitude;
    g_lon = g_stationConfig.longitude;
    ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
    g_stationConfig.cityName.c_str(), g_lat, g_lon);
  }

  // Get nearby stops for configuration interface
  getNearbyStops();
  
  // Start web server for configuration
  setupWebServer();
  
  ESP_LOGI(TAG, "Configuration mode active - web server running");
  ESP_LOGI(TAG, "Access configuration at: %s or http://mystation.local", g_stationConfig.ipAddress.c_str());
}

// --- Operational Mode ---
void runOperationalMode() {
  ESP_LOGI(TAG, "=== ENTERING OPERATIONAL MODE ===");
  
  ConfigManager& configMgr = ConfigManager::getInstance();
  
  // Set operational mode flag
  inConfigMode = false;
  configMgr.saveConfigMode(false);
  
  // Load complete configuration from NVS
  if (!configMgr.loadConfig(g_stationConfig)) {
    ESP_LOGE(TAG, "Failed to load configuration in operational mode!");
    ESP_LOGI(TAG, "Switching to configuration mode...");
    runConfigurationMode();
    return;
  }
  
  // Set coordinates from saved config
  g_lat = g_stationConfig.latitude;
  g_lon = g_stationConfig.longitude;
  ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
    g_stationConfig.cityName.c_str(), g_lat, g_lon);
  
  // Check if this is a deep sleep wake-up for fast path
  if (!configMgr.isFirstBoot() && configMgr.loadCriticalFromRTC(g_stationConfig)) {
    ESP_LOGI(TAG, "Fast wake: Using RTC config after deep sleep");
    loopCount++;
    
    // Print wakeup reason and current time
    printWakeupReason();
    ESP_LOGI(TAG, "Loop count: %lu", loopCount);
    
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
      timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  }
  
  // Connect to WiFi in station mode
  setupWiFiStationMode(g_stationConfig);
  
  if (WiFi.status() == WL_CONNECTED) {
    // Fetch and display data
    String stopIdToUse = g_stationConfig.selectedStopId.length() > 0 ? 
                         g_stationConfig.selectedStopId : 
                         (!stations.empty() ? stations[0].id : "");
    
    if (stopIdToUse.length() > 0) {
      ESP_LOGI(TAG, "Using stop ID: %s (%s)", stopIdToUse.c_str(), g_stationConfig.selectedStopName.c_str());
      getDepartureBoard(stopIdToUse.c_str());
    } else {
      ESP_LOGW(TAG, "No stop configured and no stations found.");
    }
    
    WeatherInfo weather;
    if (getWeatherFromDWD(g_lat, g_lon, weather)) {
      printWeatherInfo(weather);
    } else {
      ESP_LOGE(TAG, "Failed to get weather information from DWD.");
    }
  } else {
    ESP_LOGW(TAG, "WiFi not connected");
  }

      // Option 1: Wake up every 5 minutes (0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55)
    // uint64_t sleepTime = calculateSleepTime(1);
    
    // Option 2: Wake up every 3 minutes (0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57)
    // uint64_t sleepTime = calculateSleepTime(3);
    
    // Option 3: Wake up every 10 minutes (0, 10, 20, 30, 40, 50)
    // uint64_t sleepTime = calculateSleepTime(10);
    
    // Option 4: Wake up at specific time (e.g., 01:00 every day)
    // uint64_t sleepTime = calculateSleepUntilTime(1, 0); // 01:00
    
    // Option 5: Wake up at multiple specific times
    // if (timeinfo->tm_hour >= 22 || timeinfo->tm_hour < 6) {
    //   // Night mode: sleep until 06:00
    //   uint64_t sleepTime = calculateSleepUntilTime(6, 0);
    // } else {
    //   // Day mode: wake every 5 minutes
    //   uint64_t sleepTime = calculateSleepTime(5);
    // }

  // Calculate sleep time and enter deep sleep
  uint64_t sleepTime = calculateSleepTime(g_stationConfig.transportInterval);
  ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds", sleepTime);
  enterDeepSleep(sleepTime);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect

  esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  
  // Determine device mode based on saved configuration
  if (hasConfigInNVS()) {
    runOperationalMode();
  } else {
    runConfigurationMode();
  }
}

void loop() {
  // Only handle web server in config mode
  if (inConfigMode) {
    server.handleClient();
    delay(10); // Small delay to prevent watchdog issues
  } else {
    // Normal operation happens in setup() and then device goes to sleep
    // This should never be reached in normal operation
    ESP_LOGW(TAG, "Unexpected: loop() called in normal operation mode");
    delay(5000);
  }
}