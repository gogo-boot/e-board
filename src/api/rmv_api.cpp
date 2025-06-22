#include "rmv_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>
#include "secrets/rmv_secrets.h"
#include "../util/util.h"

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
      Serial.printf("Line: %s, Direction: %s, Time: %s, rtTime: %s, Track: %s, Category: %s\n",
            name, direction, time, rtTime, track, catOut);
    }
  } else {
    Serial.print("Failed to parse RMV departureBoard JSON: ");
    Serial.println(error.c_str());
  }
}
} // end anonymous namespace

std::vector<Station> stations;

void getNearbyStops(float lat, float lon) {
  Util::printFreeHeap("Before RMV request:");
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
      stations.clear();
      JsonArray stops = doc["stopLocationOrCoordLocation"];
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
          Serial.printf("Stop ID: %s, Name: %s, Lon: %f, Lat: %f, Type: %s\n", id, name, lon, lat, type.c_str());
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
  Util::printFreeHeap("After RMV request:");
}

void getDepartureBoard(const char* stopId) {
  HTTPClient http;
  String encodedId = Util::urlEncode(String(stopId));
  String url = "https://www.rmv.de/hapi/departureBoard?accessId=" + String(RMV_API_KEY) +
               "&id=" + encodedId +
               "&format=json&maxJourneys=10";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload); // Print full response
    printDepartures(payload);
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
