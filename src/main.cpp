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
#include "api/google_api.h" // Add this if getCityFromLatLon is declared here
#include "util/power.h"
#include "util/weather_print.h"
#include <time.h>
#include "config/config_struct.h"
#include <ESPmDNS.h>
#include "esp_sleep.h"

static const char* TAG = "MAIN";

// Function declarations
bool loadConfig(MyStationConfig &config);
void printWakeupReason();

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

// --- ESP32-C3 Super Mini Pin Definitions ---
namespace Pins {
  constexpr int A0   = 0;  // 
  constexpr int A1   = 1;  // 
  constexpr int A2   = 2;  // GPIO 2
  constexpr int A3   = 3;  // GPIO 3
  constexpr int A4   = 4;  // User Button (shares with SCK)
  constexpr int SCK  = 4;  // SPI SCK
  constexpr int A5   = 5;  // shares with MISO
  constexpr int MISO = 5;  // SPI MISO
  constexpr int MOSI = 6;  // SPI MOSI
  constexpr int SS   = 7;  // SPI SS (Chip Select)
  constexpr int SDA  = 8;  // I2C SDA
  constexpr int SCL  = 9;  // I2C SCL
  constexpr int GPIO10  = 10; // GPIO 10 (can be used for other purposes)
  constexpr int RX   = 20; // UART RX
  constexpr int TX   = 21; // UART TX

  //E-Paper Display Pins requires 6 Pins, 3.3 V, Ground. total 8 Pins used
  constexpr int EPD_BUSY = 2;  // E-Paper Busy 
  constexpr int EPD_CS = 3;  // E-Paper CS 
  constexpr int EPD_SCK = 4;  // E-Paper SCK
  constexpr int EPD_SDI = 6;  // E-Paper SDI
  constexpr int EPD_RES = 8;  // E-Paper RES
  constexpr int EPD_DC = 9;  // E-Paper D/C
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Print wakeup reason first
  printWakeupReason();

  esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  
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

  // Load saved configuration if available
  if (loadConfig(g_stationConfig)) {
    ESP_LOGI(TAG, "Loaded saved configuration");
    // If we have a valid config and not in config mode, skip WiFi setup for faster boot
    if (!inConfigMode && g_stationConfig.selectedStopId.length() > 0) {
      ESP_LOGI(TAG, "Using saved config, skipping full setup");
      return; // Skip the rest of setup for faster wake from deep sleep
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

// Print ESP32 wakeup reason
void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      ESP_LOGI(TAG, "Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      ESP_LOGI(TAG, "Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      ESP_LOGI(TAG, "Wakeup caused by ULP program");
      break;
    default:
      ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d", wakeup_reason);
      break;
  }
}

// Enhanced deep sleep function
void enterDeepSleep(uint64_t sleepTimeUs) {
  ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds", sleepTimeUs);
  
  // Configure timer wakeup
  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  
  // Optional: Configure GPIO wakeup (e.g., for user button)
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_9, 0); // Wake on low signal on GPIO 9
  
  // Power down WiFi and Bluetooth
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Flush serial output
  Serial.flush();
  
  // Enter deep sleep
  esp_deep_sleep_start();
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

    int seconds_since_hour = timeinfo->tm_min * 60 + timeinfo->tm_sec;
    int seconds_to_next = INTERVAL_SEC - (now % INTERVAL_SEC);
    ESP_LOGI(TAG, "Current time: %02d:%02d:%02d, sleeping for %d seconds to next interval.", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, seconds_to_next);
    enterDeepSleep((uint64_t)seconds_to_next * 1000000ULL);
  }
}