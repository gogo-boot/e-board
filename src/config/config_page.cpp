#include "config_page.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "api/rmv_api.h"
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "../util/util.h"
#include "esp_log.h"
#include <util/sleep_utils.h>

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
    
    // Load new configuration values
    if (doc.containsKey("weatherInterval")) config.weatherInterval = doc["weatherInterval"].as<int>();
    if (doc.containsKey("transportInterval")) config.transportInterval = doc["transportInterval"].as<int>();
    if (doc.containsKey("transportActiveStart")) config.transportActiveStart = doc["transportActiveStart"].as<String>();
    if (doc.containsKey("transportActiveEnd")) config.transportActiveEnd = doc["transportActiveEnd"].as<String>();
    if (doc.containsKey("walkingTime")) config.walkingTime = doc["walkingTime"].as<int>();
    if (doc.containsKey("sleepStart")) config.sleepStart = doc["sleepStart"].as<String>();
    if (doc.containsKey("sleepEnd")) config.sleepEnd = doc["sleepEnd"].as<String>();
    if (doc.containsKey("weekendMode")) config.weekendMode = doc["weekendMode"].as<bool>();
    if (doc.containsKey("weekendTransportStart")) config.weekendTransportStart = doc["weekendTransportStart"].as<String>();
    if (doc.containsKey("weekendTransportEnd")) config.weekendTransportEnd = doc["weekendTransportEnd"].as<String>();
    if (doc.containsKey("weekendSleepStart")) config.weekendSleepStart = doc["weekendSleepStart"].as<String>();
    if (doc.containsKey("weekendSleepEnd")) config.weekendSleepEnd = doc["weekendSleepEnd"].as<String>();
    
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
    stopsHtml += "<option value='" + encodedId + "'>" + g_stationConfig.stopNames[i] + "   ("+g_stationConfig.stopDistances[i]+"m)</option>";
  }
  stopsHtml += "<option value='__manual__'>Manuell eingeben...</option>";
  if (g_stationConfig.stopNames.size() == 0) stopsHtml = "<option>Keine Haltestellen gefunden</option>";
  page.replace("{{STOPS}}", stopsHtml);

  // Replace city, ssid, etc.
  page.replace("{{CITY}}", g_stationConfig.cityName);
  // Separate Router (SSID) and IP info
  page.replace("{{ROUTER}}", g_stationConfig.ssid);
  page.replace("{{IP}}", g_stationConfig.ipAddress); // Replace with IP info if available
  page.replace("{{MDNS}}", ".local"); // mDNS hostname
  
  // Replace configuration values with current settings
  page.replace("{{WEATHER_INTERVAL}}", String(g_stationConfig.weatherInterval));
  page.replace("{{TRANSPORT_INTERVAL}}", String(g_stationConfig.transportInterval));
  page.replace("{{TRANSPORT_ACTIVE_START}}", g_stationConfig.transportActiveStart);
  page.replace("{{TRANSPORT_ACTIVE_END}}", g_stationConfig.transportActiveEnd);
  page.replace("{{WALKING_TIME}}", String(g_stationConfig.walkingTime));
  page.replace("{{SLEEP_START}}", g_stationConfig.sleepStart);
  page.replace("{{SLEEP_END}}", g_stationConfig.sleepEnd);
  page.replace("{{WEEKEND_MODE}}", g_stationConfig.weekendMode ? "checked" : "");
  page.replace("{{WEEKEND_TRANSPORT_START}}", g_stationConfig.weekendTransportStart);
  page.replace("{{WEEKEND_TRANSPORT_END}}", g_stationConfig.weekendTransportEnd);
  page.replace("{{WEEKEND_SLEEP_START}}", g_stationConfig.weekendSleepStart);
  page.replace("{{WEEKEND_SLEEP_END}}", g_stationConfig.weekendSleepEnd);
  
  // Build JavaScript array for saved filters
  String filtersJs = "[";
  for (size_t i = 0; i < g_stationConfig.oepnvFilters.size(); i++) {
    if (i > 0) filtersJs += ",";
    filtersJs += "\"" + g_stationConfig.oepnvFilters[i] + "\"";
  }
  filtersJs += "]";
  page.replace("{{SAVED_FILTERS}}", filtersJs);

  server.send(200, "text/html; charset=utf-8", page);
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer &server,bool &inConfigMode) {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    extern MyStationConfig g_stationConfig;
    ConfigManager& configMgr = ConfigManager::getInstance();
    
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

    // Update global config structure
    if (doc.containsKey("city")) g_stationConfig.cityName = doc["city"].as<String>();
    if (doc.containsKey("cityLat")) g_stationConfig.latitude = doc["cityLat"].as<float>();
    if (doc.containsKey("cityLon")) g_stationConfig.longitude = doc["cityLon"].as<float>();
    if (doc.containsKey("stopId")) g_stationConfig.selectedStopId = doc["stopId"].as<String>();
    if (doc.containsKey("stopName")) g_stationConfig.selectedStopName = doc["stopName"].as<String>();
    
    // Update ÖPNV filters
    if (doc.containsKey("filters")) {
        g_stationConfig.oepnvFilters.clear();
        JsonArray filters = doc["filters"];
        for (JsonVariant v : filters) {
            g_stationConfig.oepnvFilters.push_back(v.as<String>());
        }
    }
    
    // Update new configuration values
    if (doc.containsKey("weatherInterval")) g_stationConfig.weatherInterval = doc["weatherInterval"].as<int>();
    if (doc.containsKey("transportInterval")) g_stationConfig.transportInterval = doc["transportInterval"].as<int>();
    if (doc.containsKey("transportActiveStart")) g_stationConfig.transportActiveStart = doc["transportActiveStart"].as<String>();
    if (doc.containsKey("transportActiveEnd")) g_stationConfig.transportActiveEnd = doc["transportActiveEnd"].as<String>();
    if (doc.containsKey("walkingTime")) g_stationConfig.walkingTime = doc["walkingTime"].as<int>();
    if (doc.containsKey("sleepStart")) g_stationConfig.sleepStart = doc["sleepStart"].as<String>();
    if (doc.containsKey("sleepEnd")) g_stationConfig.sleepEnd = doc["sleepEnd"].as<String>();
    if (doc.containsKey("weekendMode")) g_stationConfig.weekendMode = doc["weekendMode"].as<bool>();
    if (doc.containsKey("weekendTransportStart")) g_stationConfig.weekendTransportStart = doc["weekendTransportStart"].as<String>();
    if (doc.containsKey("weekendTransportEnd")) g_stationConfig.weekendTransportEnd = doc["weekendTransportEnd"].as<String>();
    if (doc.containsKey("weekendSleepStart")) g_stationConfig.weekendSleepStart = doc["weekendSleepStart"].as<String>();
    if (doc.containsKey("weekendSleepEnd")) g_stationConfig.weekendSleepEnd = doc["weekendSleepEnd"].as<String>();
    
    // Save to NVS (and RTC memory automatically)
    if (!configMgr.saveConfig(g_stationConfig)) {
        server.send(500, "text/plain", "Failed to save config to NVS");
        return;
    }
    
    inConfigMode = false;
    
    // Save config mode to NVS (persists across power loss)
    configMgr.saveConfigMode(false);
    
    // Switch to station mode only
    #ifdef ESP32
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
    #endif
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");

    enterDeepSleep(1 * 1000000); // Enter deep sleep for 1 seconds
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
