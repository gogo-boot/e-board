#include "google_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "secrets/google_secrets.h"

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

bool getLocationFromGoogle(float &lat, float &lon) {
  String wifiJson = buildWifiJson();
  HTTPClient http;
  String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + String(GOOGLE_API_KEY);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(wifiJson);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload); // Debug print
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      lat = doc["location"]["lat"];
      lon = doc["location"]["lng"];
      http.end();
      return true;
    }
  }
  http.end();
  return false;
}
