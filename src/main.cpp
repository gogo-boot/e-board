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

WebServer server(80);
bool inConfigMode = true;

float g_lat = 0.0, g_lon = 0.0;

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed! Please check filesystem or flash.");
    while (true) { delay(1000); }
  }
  WiFiManager wm;
  wm.resetSettings(); // Reset WiFi settings for fresh start
  // Debug: Print before AP setup
  Serial.println("[DEBUG] Starting WiFiManager AP mode...");
  const char* menu[] = { "wifi" };
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  String apName = Util::getUniqueSSID("MyStation");
  Serial.print("[DEBUG] AP SSID: "); Serial.println(apName);
  bool res = wm.autoConnect(apName.c_str());
  Serial.println("[DEBUG] autoConnect() returned");
  if(!res) {
      Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi connected!");
    // WiFi.mode(WIFI_STA); // Ensure STA mode
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
    getLocationFromGoogle(g_lat, g_lon);
    getNearbyStops(g_lat, g_lon);
    Serial.println("connected...yeey :)");
    server.on("/", [](){ handleConfigPage(server); });
    server.on("/done", [](){ handleConfigDone(server, inConfigMode); });
    static std::vector<Station> stations; // Ensure Station type is defined and included
    server.on("/stations", [&](){ handleStationSelect(server, stations); });
    server.begin();
    Serial.println("HTTP server started.");
  }
}

void loop() {
  if (inConfigMode) {

    server.handleClient();
    // When user finishes config, they should visit /done to exit config mode
  } else {
    static unsigned long loopCount = 0;
    unsigned long startTime = millis();
    loopCount++;
    Serial.printf("Loop count: %lu\n", loopCount);
    if (WiFi.status() == WL_CONNECTED) {
        //Todo: Replace with actual stopId from getNearbyStops
        // getDepartureBoard("A=1@O=Frankfurt (Main) Rödelheim Bahnhof@X=8606947@Y=50125164@U=80@L=3001217@"); // Example stopId, replace with actual ID from getNearbyStops
        getDepartureBoard("A=1@O=Frankfurt (Main) Radilostraße@X=8610722@Y=50125083@U=80@L=3001238@"); // Example stopId, replace with actual ID from getNearbyStops
        // Call weather API
        WeatherInfo weather;
        if (getWeatherFromDWD(g_lat, g_lon, weather)) {
            Serial.printf("Weather for %s: %s, %s\n", weather.city.c_str(), weather.temperature.c_str(), weather.condition.c_str());
        } else {
            Serial.println("Failed to get weather information from DWD.");
        }
    } else {
      Serial.println("WiFi not connected");
    }
    unsigned long endTime = millis();
    unsigned long duration = endTime - startTime;
    Serial.printf("Loop duration: %lu ms\n", duration);
    delay(60000); // Wait 60 seconds before next request
  }
}
