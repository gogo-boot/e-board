#define ARDUINOJSON_DECODE_NESTING_LIMIT 200
#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "secrets/secrets.h"
#include "secrets/rmv_secrets.h"
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>

WebServer server(80);
bool inConfigMode = true;

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
    DynamicJsonDocument doc(1024); // Use heap, not stack
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

struct Station {
  String id;
  String name;
  String type; // "bus" or "train"
};
std::vector<Station> stations; // Fill from RMV API

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
  printFreeHeap("After RMV request:");
}

String urlEncode(const String& str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      code0 = ((c >> 4) & 0xf) + '0';
      if (((c >> 4) & 0xf) > 9) code0 = ((c >> 4) & 0xf) - 10 + 'A';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

// Filter for only needed fields in Departure
StaticJsonDocument<256> departureFilter;
void initDepartureFilter() {
  departureFilter["Departure"][0]["name"] = true;
  departureFilter["Departure"][0]["direction"] = true;
  departureFilter["Departure"][0]["track"] = true;  // Optional, may not be present
  departureFilter["Departure"][0]["rtTime"] = true;  // not always present, use rtTime if available
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

void getDepartureBoard(const char* stopId) {
  HTTPClient http;
  String encodedId = urlEncode(String(stopId));
  String url = "https://www.rmv.de/hapi/departureBoard?accessId=" + String(RMV_API_KEY) +
               "&id=" + encodedId +
              //  "&duration=5" + // Duration in minutes
              //  "&minDur=15" + // Duration in minutes
               "&format=json&maxJourneys=10"; // Limit to 10 journeys
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

String generateStationsHTML(const std::vector<Station>& stations) {
  String html;
  for (size_t i = 0; i < stations.size(); ++i) {
    const auto& s = stations[i];
    html += "<div class='station'>";
    html += "<input type='radio' name='station' value='" + s.id + "'>";
    html += s.name + " (" + s.type + ")";
    html += "</div>";
  }
  return html;
}

void handleStationSelect() {
  File file = LittleFS.open("/station_select.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Template not found");
    return;
  }
  String page = file.readString();
  file.close();
  page.replace("{{stations}}", generateStationsHTML(stations));
  server.send(200, "text/html", page);
}

void handleConfigPage() {
  server.send(200, "text/html", "<h1>ESP32 Configuration Page</h1><p>Put your config form here.</p>");
}

void handleConfigDone() {
  inConfigMode = false;
  server.send(200, "text/html", "<h1>Configuration Complete</h1><p>Device will now run in normal mode.</p>");
}

String getUniqueSSID(const String& prefix) {
  uint32_t chipId = (uint32_t)ESP.getEfuseMac(); // Unique per ESP32
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "%s-%06X", prefix.c_str(), chipId & 0xFFFFFF);
  return String(ssid);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, PlatformIO World!");
  // if (!LittleFS.begin()) {
  //   Serial.println("[ERROR] LittleFS mount failed! Please check filesystem or flash.");
  //   while (true) { delay(1000); }
  // }
  WiFiManager wm;
  wm.resetSettings(); // Reset WiFi settings for fresh start
  // Debug: Print before AP setup
  Serial.println("[DEBUG] Starting WiFiManager AP mode...");
  const char* menu[] = { "wifi" };
  wm.setMenu(menu, 1);
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  String apName = getUniqueSSID("MyStation");
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
    float lat = 0, lon = 0;
      getLocationFromGoogle(lat, lon);
      getNearbyStops(lat, lon);
        
      Serial.println("connected...yeey :)");
      server.on("/", handleConfigPage);
      server.on("/done", handleConfigDone); // Call this URL to finish config
      server.on("/stations", handleStationSelect);
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
    } else {
      Serial.println("WiFi not connected");
    }
    unsigned long endTime = millis();
    unsigned long duration = endTime - startTime;
    Serial.printf("Loop duration: %lu ms\n", duration);
    delay(60000); // Wait 60 seconds before next request
  }
}
