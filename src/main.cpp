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

static const char* TAG = "MAIN";

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

  esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  // Example: set config values
  g_stationConfig.latitude = 0.0; // will be set after Google API call
  g_stationConfig.longitude = 0.0; // will be set after Google API call
  g_stationConfig.ssid = ""; // will be set after WiFi connects
  g_stationConfig.cityName = ""; //will be set after city lookup
  g_stationConfig.oepnvFilters = {"RE", "S-Bahn", "Bus"};

  WiFiManager wm;
  // wm.resetSettings(); // Reset WiFi settings for fresh start

  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
    while (true) {
      delay(1000);
    }
  }

  setupWiFiAndMDNS(wm, g_stationConfig);
  setupTime();
  getLocationFromGoogle(g_lat, g_lon); // This will set g_stationConfig.latitude/longitude 
  getCityFromLatLon(g_lat, g_lon);
  ESP_LOGI(TAG, "City set in setup: %s", g_stationConfig.cityName.c_str());
  getNearbyStops(); // Now uses g_stationConfig for lat/lon
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
      if (!stations.empty()) {
        String firstStationId = stations[0].id;  
        ESP_LOGI(TAG, "First station ID: %s", firstStationId.c_str());
        getDepartureBoard(firstStationId.c_str()); // Todo: pass stopId from config
      } else {
        ESP_LOGW(TAG, "No stations found from getNearbyStops.");
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
    enterHibernate((uint64_t)seconds_to_next * 1000000ULL);
  }
}