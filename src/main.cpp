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

// --- Globals ---
WebServer server(80);
RTC_DATA_ATTR bool inConfigMode = true;
RTC_DATA_ATTR unsigned long loopCount = 0;

float g_lat = 0.0, g_lon = 0.0;
MyStationConfig g_stationConfig;

// --- Modularized Setup Functions ---
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

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Print wakeup reason first
  printWakeupReason();

  esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  
  ConfigManager& configMgr = ConfigManager::getInstance();
  
  // Fast path: Try to load critical config from RTC memory first (after deep sleep)
  if (!configMgr.isFirstBoot() && configMgr.loadCriticalFromRTC(g_stationConfig)) {
    ESP_LOGI(TAG, "Fast wake: Using RTC config");
    // We have the essential data, skip full setup for speed
    if (!inConfigMode && g_stationConfig.selectedStopId.length() > 0) {
      ESP_LOGI(TAG, "Fast wake complete, skipping full setup");
      return; // Ultra-fast wake for regular operation
    }
  }
  
  // Slow path: Full boot or first-time setup
  ESP_LOGI(TAG, "Full boot: Loading complete configuration");
  
  // Initialize config with defaults
  g_stationConfig.latitude = 0.0;
  g_stationConfig.longitude = 0.0;
  g_stationConfig.ssid = "";
  g_stationConfig.cityName = "";
  g_stationConfig.oepnvFilters = {"RE", "S-Bahn", "Bus"};

  WiFiManager wm;
  // wm.resetSettings(); // Reset WiFi settings for fresh start

  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
    while (true) {
      delay(1000);
    }
  }

  // Load complete configuration from NVS (slower but complete)
  if (configMgr.loadConfig(g_stationConfig)) {
    ESP_LOGI(TAG, "Loaded complete configuration from NVS");
    // If we have a valid config and not in config mode, we can still optimize
    if (!inConfigMode && g_stationConfig.selectedStopId.length() > 0) {
      ESP_LOGI(TAG, "Using saved config, minimal setup");
      // Still need WiFi for API calls, but can skip location lookup
      setupWiFiAndMDNS(wm, g_stationConfig);
      return;
    }
  } else {
    ESP_LOGI(TAG, "No saved config found, entering config mode");
    inConfigMode = true;
  }

  setupWiFiAndMDNS(wm, g_stationConfig);
  setupTime();
  
  // Only fetch location if we don't have it saved
  if (g_stationConfig.latitude == 0.0 && g_stationConfig.longitude == 0.0) {
    getLocationFromGoogle(g_lat, g_lon);
    getCityFromLatLon(g_lat, g_lon);
    ESP_LOGI(TAG, "City set in setup: %s", g_stationConfig.cityName.c_str());
    getNearbyStops();
  } else {
    // Use saved coordinates
    g_lat = g_stationConfig.latitude;
    g_lon = g_stationConfig.longitude;
    ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
             g_stationConfig.cityName.c_str(), g_lat, g_lon);
  }

  setupWebServer();
}

void loop() {
  // --- Main Loop: Handles config mode and periodic updates ---
  const int INTERVAL_SEC = 60; // 1 minute

  if (inConfigMode) {
    // Handle web server requests in config mode
    server.handleClient();
    // When user finishes config, they should visit /done to exit config mode
  } else {
    loopCount++;
    // Print current time from NTP/RTC
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
      timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    if (WiFi.status() == WL_CONNECTED) {
      // --- Main Data Fetch: Departure board and weather ---
      // Use configured stop ID if available, otherwise fall back to first station
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

    // Choose your preferred wakeup strategy:
    
    // Option 1: Wake up every 5 minutes (0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55)
    uint64_t sleepTime = calculateSleepTime(5);
    
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
    
    enterDeepSleep(sleepTime);
  }
}