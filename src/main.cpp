#define ARDUINOJSON_DECODE_NESTING_LIMIT 200
#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
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
#include "util/power.h"
#include "util/weather_print.h"
#include <time.h>
#include "esp_log.h"

static const char* TAG = "MAIN";

WebServer server(80);
RTC_DATA_ATTR bool inConfigMode = true;
RTC_DATA_ATTR unsigned long loopCount = 0;

float g_lat = 0.0, g_lon = 0.0;

void setup()
{
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_INFO); // Set global log level
  ESP_LOGI(TAG, "System starting...");
  WiFiManager wm;
  // wm.resetSettings(); // Reset WiFi settings for fresh start
  ESP_LOGD(TAG, "Starting WiFiManager AP mode...");
  const char *menu[] = {"wifi"};
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
  String apName = Util::getUniqueSSID("MyStation");
  ESP_LOGD(TAG, "AP SSID: %s", apName.c_str());
  bool res = wm.autoConnect(apName.c_str());
  ESP_LOGD(TAG, "autoConnect() returned");

  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
    while (true) {
      delay(1000);
    }
  }

  if (!res) {
    ESP_LOGE(TAG, "Failed to connect");
  } else {
    ESP_LOGI(TAG, "WiFi connected!");
    // WiFi.mode(WIFI_STA); // Ensure STA mode
    ESP_LOGI(TAG, "ESP32 IP address: %s", WiFi.localIP().toString().c_str());

    // NTP time sync
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

    getLocationFromGoogle(g_lat, g_lon);
    getNearbyStops(g_lat, g_lon);

    server.on("/", []()
              { handleConfigPage(server); });
    server.on("/done", []()
              { handleConfigDone(server, inConfigMode); });
    server.on("/save_config", HTTP_POST, []() { handleSaveConfig(server, inConfigMode ); });
    server.on("/api/stop", HTTP_GET, []() { handleStopAutocomplete(server); });
    server.begin();
    ESP_LOGI(TAG, "HTTP server started.");
  }
}

void loop() {
  const int INTERVAL_SEC = 60; // 1 minute

  if (inConfigMode) {
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
      if (!stations.empty()) {
        String firstStationId = stations[0].id;
        ESP_LOGI(TAG, "First station ID: %s", firstStationId.c_str());
        getDepartureBoard(firstStationId.c_str());
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