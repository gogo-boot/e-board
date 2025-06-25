#include "rmv_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>
#include "secrets/rmv_secrets.h"
#include "../util/util.h"
#include "esp_log.h"
#include "config/config_struct.h"

static const char* TAG = "RMV_API";

namespace {
StaticJsonDocument<256> departureFilter;
void initDepartureFilter() {
  departureFilter["Departure"][0]["name"] = true;
  departureFilter["Departure"][0]["direction"] = true;
  departureFilter["Departure"][0]["track"] = true;
  departureFilter["Departure"][0]["rtTime"] = true;
  departureFilter["Departure"][0]["time"] = true;
  departureFilter["Departure"][0]["Product"][0]["catOut"] = true;
}

void printDepartures(const String& payload) {
  initDepartureFilter();
  DynamicJsonDocument doc(20480);
  DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(departureFilter));
  if (!error) {
    JsonArray departures = doc["Departure"];
    for (JsonObject dep : departures) {
      const char* name = dep["name"] | "";
      const char* direction = dep["direction"] | "";
      const char* time = dep["time"] | "";
      const char* rtTime = dep["rtTime"] | "";
      const char* track = dep["track"] | "";
      const char* catOut = "";
      if (dep.containsKey("Product") && dep["Product"].is<JsonArray>() && dep["Product"].size() > 0) {
        catOut = dep["Product"][0]["catOut"] | "";
      }
      ESP_LOGI(TAG, "Line: %s, Direction: %s, Time: %s, rtTime: %s, Track: %s, Category: %s",
            name, direction, time, rtTime, track, catOut);
    }
  } else {
    ESP_LOGE(TAG, "Failed to parse RMV departureBoard JSON: %s", error.c_str());
  }
}
} // end anonymous namespace

std::vector<Station> stations;

void getNearbyStops() {
  Util::printFreeHeap("Before RMV request:");
  extern MyStationConfig g_stationConfig;
  float lat = g_stationConfig.latitude;
  float lon = g_stationConfig.longitude;
  HTTPClient http;
  String url = "https://www.rmv.de/hapi/location.nearbystops?accessId=" + String(RMV_API_KEY) +
               "&originCoordLat=" + String(lat, 6) +
               "&originCoordLong=" + String(lon, 6) +
               "&format=json&maxNo=7";
  String urlForLog = url;
  int keyPos = urlForLog.indexOf("accessId=");
  if (keyPos != -1) {
    int keyEnd = urlForLog.indexOf('&', keyPos);
    if (keyEnd == -1) keyEnd = urlForLog.length();
    urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
  }
  ESP_LOGI(TAG, "Requesting nearby stops: %s", urlForLog.c_str());
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    ESP_LOGD(TAG, "Nearby stops response: %s", payload.c_str());
    DynamicJsonDocument doc(8192); // Use heap, not stack
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      stations.clear();
      g_stationConfig.stopIds.clear();
      g_stationConfig.stopNames.clear();
      JsonArray stops = doc["stopLocationOrCoordLocation"];
      extern MyStationConfig g_stationConfig;
      for (JsonObject item : stops) {
        JsonObject stop = item["StopLocation"];
        if (!stop.isNull()) {
          const char* id = stop["id"] | "";
          const char* name = stop["name"] | "";
          float lon = stop["lon"] | 0.0;
          float lat = stop["lat"] | 0.0;
          int products = stop["products"] | 0;
          String type = (products & 64) ? "train" : "bus"; // Example: RMV uses bitmask for products
          stations.push_back({String(id), String(name), type});
          g_stationConfig.stopIds.push_back(id);
          g_stationConfig.stopNames.push_back(name);
          ESP_LOGI(TAG, "Stop ID: %s, Name: %s, Lon: %f, Lat: %f, Type: %s", id, name, lon, lat, type.c_str());
        }
      }
    } else {
      ESP_LOGE(TAG, "Failed to parse RMV JSON: %s", error.c_str());
    }
  } else {
    ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
  }
  http.end();
  Util::printFreeHeap("After RMV request:");
}

void getDepartureBoard(const char* stopId) {
  HTTPClient http;
  String encodedId = Util::urlEncode(String(stopId));
  String url = "https://www.rmv.de/hapi/departureBoard?accessId=" + String(RMV_API_KEY) +
               "&id=" + encodedId +
               "&format=json&maxJourneys=10";
  String urlForLog = url;
  int keyPos = urlForLog.indexOf("accessId=");
  if (keyPos != -1) {
    int keyEnd = urlForLog.indexOf('&', keyPos);
    if (keyEnd == -1) keyEnd = urlForLog.length();
    urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
  }
  ESP_LOGI(TAG, "Requesting departure board: %s", urlForLog.c_str());
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    ESP_LOGD(TAG, "Departure board response: %s", payload.c_str());
    printDepartures(payload);
  } else {
    ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
  }
  http.end();
}
