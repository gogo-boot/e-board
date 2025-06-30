#include "config_page.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "api/rmv_api.h"
#include "config/config_struct.h"
#include "../util/util.h"
#include "esp_log.h"

static const char* TAG = "CONFIG";

extern float g_lat, g_lon;

// Load configuration from JSON file into g_stationConfig
bool loadConfig(MyStationConfig &config) {
    if (!LittleFS.exists("/config.json")) {
        ESP_LOGW(TAG, "Config file not found, using defaults");
        return false;
    }
    
    File f = LittleFS.open("/config.json", "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open config file");
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    
    if (err) {
        ESP_LOGE(TAG, "Failed to parse config JSON: %s", err.c_str());
        return false;
    }
    
    // Load values from JSON into config struct
    if (doc.containsKey("city")) config.cityName = doc["city"].as<String>();
    if (doc.containsKey("cityLat")) config.latitude = doc["cityLat"].as<float>();
    if (doc.containsKey("cityLon")) config.longitude = doc["cityLon"].as<float>();
    if (doc.containsKey("stopId")) config.selectedStopId = doc["stopId"].as<String>();
    if (doc.containsKey("stopName")) config.selectedStopName = doc["stopName"].as<String>();
    
    // Load ÖPNV filters
    if (doc.containsKey("filters")) {
        config.oepnvFilters.clear();
        JsonArray filters = doc["filters"];
        for (JsonVariant v : filters) {
            config.oepnvFilters.push_back(v.as<String>());
        }
    }
    
    ESP_LOGI(TAG, "Config loaded: City=%s, Stop=%s, Filters=%d", 
             config.cityName.c_str(), config.selectedStopName.c_str(), config.oepnvFilters.size());
    return true;
}

void handleStationSelect(WebServer &server) {
    extern MyStationConfig g_stationConfig;
    File file = LittleFS.open("/station_select.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Template not found");
        return;
    }
    String page = file.readString();
    file.close();
    String html;
    for (size_t i = 0; i < g_stationConfig.stopNames.size(); ++i) {
        html += "<div class='station'>";
        html += "<input type='radio' name='station' value='" + g_stationConfig.stopIds[i] + "'>";
        html += g_stationConfig.stopNames[i] + " (ID: " + g_stationConfig.stopIds[i] + ")";
        html += "</div>";
    }
    page.replace("{{stations}}", html);
    server.send(200, "text/html", page);
}

// Update the config page handler to serve config_my_station.html
void handleConfigPage(WebServer &server) {
  extern MyStationConfig g_stationConfig;
  File file = LittleFS.open("/config_my_station.html", "r");
  if (!file) {
    server.send(404, "text/plain", "Konfigurationsseite nicht gefunden");
    return;
  }
  String page = file.readString();
  file.close();

  // Replace reserved keywords
  page.replace("{{LAT}}", String(g_stationConfig.latitude, 6));
  page.replace("{{LON}}", String(g_stationConfig.longitude, 6));

  // Build <option> list for stops, add manual entry option
  String stopsHtml = "<option value=''>Bitte wählen...</option>";
  for (size_t i = 0; i < g_stationConfig.stopNames.size(); ++i) {
    String encodedId = Util::urlEncode(g_stationConfig.stopIds[i]);
    stopsHtml += "<option value='" + encodedId + "'>" + g_stationConfig.stopNames[i] + "</option>";
  }
  stopsHtml += "<option value='__manual__'>Manuell eingeben...</option>";
  if (g_stationConfig.stopNames.size() == 0) stopsHtml = "<option>Keine Haltestellen gefunden</option>";
  page.replace("{{STOPS}}", stopsHtml);

  // Replace city, ssid, etc.
  page.replace("{{CITY}}", g_stationConfig.cityName);
  // Separate Router (SSID) and IP info
  page.replace("{{ROUTER}}", g_stationConfig.ssid);
  page.replace("{{IP}}", g_stationConfig.ipAddress); // Replace with IP info if available

  server.send(200, "text/html; charset=utf-8", page);
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer &server,bool &inConfigMode) {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    // Parse JSON body
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    // Decode stopId if present
    if (doc.containsKey("stopId")) {
        String stopId = doc["stopId"].as<String>();
        doc["stopId"] = Util::urlDecode(stopId);
    }
    // Print the entire doc object for debugging
    String docStr;
    serializeJsonPretty(doc, docStr);
    ESP_LOGI("CONFIG", "[Config] Received JSON:\n%s", docStr.c_str());

    File f = LittleFS.open("/config.json", "w");
    if (!f) {
        server.send(500, "text/plain", "Failed to save config");
        return;
    }
    serializeJson(doc, f);
    f.close();
    inConfigMode = false;
    // Switch to station mode only
    #ifdef ESP32
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
    #endif
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// AJAX handler to resolve station/stop name (GET /api/stop?q=...)
void handleStopAutocomplete(WebServer &server) {
    String query = server.hasArg("q") ? server.arg("q") : "";
    // TODO: Call RMV API and return JSON array of suggestions
    // For now, return dummy data
    DynamicJsonDocument doc(256);
    JsonArray arr = doc.to<JsonArray>();
    arr.add("Frankfurt Hauptbahnhof");
    arr.add("Frankfurt West");
    arr.add("Frankfurt Süd");
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}
