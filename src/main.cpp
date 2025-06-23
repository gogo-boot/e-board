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
#include "esp_sleep.h"
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

void enterHibernate(uint64_t sleep_us) {
  Serial.printf("Entering hibernate mode for %.2f minutes...\n", sleep_us / 60000000.0);
  esp_sleep_enable_timer_wakeup(sleep_us);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
  // The device will reset after wakeup
}

void printWeatherInfo(const WeatherInfo &weather) {
  Serial.println("--- WeatherInfo ---");
  Serial.printf("City: %s\n", weather.city.c_str());
  Serial.printf("Current Temp: %s\n", weather.temperature.c_str());
  Serial.printf("Condition: %s\n", weather.condition.c_str());
  Serial.printf("Max Temp: %s\n", weather.tempMax.c_str());
  Serial.printf("Min Temp: %s\n", weather.tempMin.c_str());
  Serial.printf("Sunrise: %s\n", weather.sunrise.c_str());
  Serial.printf("Sunset: %s\n", weather.sunset.c_str());
  Serial.printf("Raw JSON: %s\n", weather.rawJson.c_str());
  Serial.printf("Forecast count: %d\n", weather.forecastCount);
  for (int i = 0; i < weather.forecastCount && i < 12; ++i) {
    const auto &hour = weather.forecast[i];
    Serial.printf("-- Hour %d --\n", i + 1);
    Serial.printf("Time: %s\n", hour.time.c_str());
    Serial.printf("Temp: %s\n", hour.temperature.c_str());
    Serial.printf("Rain Chance: %s\n", hour.rainChance.c_str());
    Serial.printf("Humidity: %s\n", hour.humidity.c_str());
    Serial.printf("Wind Speed: %s\n", hour.windSpeed.c_str());
    Serial.printf("Rainfall: %s\n", hour.rainfall.c_str());
    Serial.printf("Snowfall: %s\n", hour.snowfall.c_str());
    Serial.printf("Weather Code: %s\n", hour.weatherCode.c_str());
    Serial.printf("Weather Desc: %s\n", hour.weatherDesc.c_str());
  }
  Serial.println("--- End WeatherInfo ---");
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
