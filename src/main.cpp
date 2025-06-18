#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "secrets/secrets.h"
#include "secrets/rmv_secrets.h"

// Utility: Print free heap
void printFreeHeap(const char* msg) {
  Serial.printf("%s Free heap: %u bytes\n", msg, ESP.getFreeHeap());
}

// Scan WiFi and build JSON for Google Geolocation API
String buildWifiJson() {
  StaticJsonDocument<1024> doc;
  doc["considerIp"] = false;
  JsonArray aps = doc.createNestedArray("wifiAccessPoints");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    JsonObject ap = aps.createNestedObject();
    ap["macAddress"] = WiFi.BSSIDstr(i);
    ap["signalStrength"] = WiFi.RSSI(i);
    ap["signalToNoiseRatio"] = 0;
  }
  WiFi.scanDelete();
  String output;
  serializeJson(doc, output);
  return output;
}

// Get location from Google Geolocation API
bool getLocationFromGoogle(float &lat, float &lon) {
  String wifiJson = buildWifiJson();
  HTTPClient http;
  String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + String(GOOGLE_API_KEY);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(wifiJson);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      lat = doc["location"]["lat"];
      lon = doc["location"]["lng"];
      Serial.printf("Google Location: Latitude: %f, Longitude: %f\n", lat, lon);
      http.end();
      return true;
    } else {
      Serial.println("Failed to parse Google JSON response");
    }
  } else {
    Serial.printf("Google HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return false;
}

// Get nearby stops from RMV API
void getNearbyStops(float lat, float lon) {
  printFreeHeap("Before RMV request:");
  HTTPClient http;
  String url = "https://www.rmv.de/hapi/location.nearbystops?accessId=" + String(RMV_API_KEY) +
               "&originCoordLat=" + String(lat, 6) +
               "&originCoordLong=" + String(lon, 6) +
               "&format=json&maxNo=7";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload); // Print full response
    DynamicJsonDocument doc(8192); // Use heap, not stack
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      JsonArray stops = doc["stopLocationOrCoordLocation"];
      for (JsonObject item : stops) {
        JsonObject stop = item["StopLocation"];
        if (!stop.isNull()) {
          const char* id = stop["id"] | "";
          const char* name = stop["name"] | "";
          float lon = stop["lon"] | 0.0;
          float lat = stop["lat"] | 0.0;
          Serial.printf("Stop ID: %s, Name: %s, Lon: %f, Lat: %f\n", id, name, lon, lat);
        }
      }
    } else {
      Serial.print("Failed to parse RMV JSON: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  printFreeHeap("After RMV request:");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, PlatformIO World!");
  WiFiManager wm;
  const char* menu[] = { "wifi" };
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // wm.resetSettings(); // Uncomment to clear WiFi credentials
  bool res = wm.autoConnect("MyStation");
  if(!res) {
      Serial.println("Failed to connect");
  } else {
      Serial.println("connected...yeey :)");
  }
}

void loop() {
  static unsigned long loopCount = 0;
  loopCount++;
  Serial.printf("Loop count: %lu\n", loopCount);
  if (WiFi.status() == WL_CONNECTED) {
    float lat = 0, lon = 0;
    if (getLocationFromGoogle(lat, lon)) {
      getNearbyStops(lat, lon);
    }
  } else {
    Serial.println("WiFi not connected");
  }
  delay(60000); // Wait 60 seconds before next request
}
