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

WebServer server(80);
RTC_DATA_ATTR bool inConfigMode = true;
RTC_DATA_ATTR unsigned long loopCount = 0;

float g_lat = 0.0, g_lon = 0.0;

void setup()
{
  Serial.begin(115200);
  WiFiManager wm;
  // wm.resetSettings(); // Reset WiFi settings for fresh start
  // Debug: Print before AP setup
  Serial.println("[DEBUG] Starting WiFiManager AP mode...");
  const char *menu[] = {"wifi"};
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
  String apName = Util::getUniqueSSID("MyStation");
  Serial.print("[DEBUG] AP SSID: ");
  Serial.println(apName);
  bool res = wm.autoConnect(apName.c_str());
  Serial.println("[DEBUG] autoConnect() returned");

  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed! Please check filesystem or flash.");
    while (true) {
      delay(1000);
    }
  }

  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi connected!");
    // WiFi.mode(WIFI_STA); // Ensure STA mode
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());

    // NTP time sync
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync");
    time_t now = time(nullptr);
    int retry = 0;
    const int retry_count = 30;
    while (now < 8 * 3600 * 2 && retry < retry_count) { // year < 1971
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      retry++;
    }
    Serial.println();
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.printf("NTP time set: %04d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    getLocationFromGoogle(g_lat, g_lon);
    getNearbyStops(g_lat, g_lon);

    Serial.println("connected...yeey :)");
    server.on("/", []()
              { handleConfigPage(server); });
    server.on("/done", []()
              { handleConfigDone(server, inConfigMode); });
    server.on("/stations", [&]()
              { handleStationSelect(server, stations); });
    server.begin();
    Serial.println("HTTP server started.");
  }
}

void loop() {
  // Set your interval in seconds (e.g., 3600 for hourly, 10800 for 3-hourly)
  const int INTERVAL_SEC = 60; // 1 minute

  if (inConfigMode) {
    server.handleClient();
    // When user finishes config, they should visit /done to exit config mode
  } else {
    loopCount++;
    // Print current time from NTP/RTC
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    if (WiFi.status() == WL_CONNECTED) {
      if (!stations.empty()) {
        String firstStationId = stations[0].id;
        Serial.printf("First station ID: %s\n", firstStationId.c_str());
        getDepartureBoard(firstStationId.c_str());
      } else {
        Serial.println("No stations found from getNearbyStops.");
      }
      WeatherInfo weather;
      if (getWeatherFromDWD(g_lat, g_lon, weather)) {
        printWeatherInfo(weather);
      } else {
        Serial.println("Failed to get weather information from DWD.");
      }
    } else {
      Serial.println("WiFi not connected");
    }

    // Calculate seconds to next interval using the already declared now/timeinfo
    int seconds_since_hour = timeinfo->tm_min * 60 + timeinfo->tm_sec;
    int seconds_to_next = INTERVAL_SEC - (now % INTERVAL_SEC);
    Serial.printf("Current time: %02d:%02d:%02d, sleeping for %d seconds to next interval.\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, seconds_to_next);
    enterHibernate((uint64_t)seconds_to_next * 1000000ULL);
  }
}
