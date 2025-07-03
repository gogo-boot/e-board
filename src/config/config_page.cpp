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

  // Get current configuration from RTC memory
  RTCConfigData& config = ConfigManager::getConfig();

  // Replace reserved keywords with RTC config data
  page.replace("{{LAT}}", String(config.latitude, 6));
  page.replace("{{LON}}", String(config.longitude, 6));

  // Build <option> list for stops, add manual entry option
  String stopsHtml = "<option value=''>Bitte wählen...</option>";
  for (size_t i = 0; i < g_stationConfig.stopNames.size(); ++i) {
    String encodedId = Util::urlEncode(g_stationConfig.stopIds[i]);
    stopsHtml += "<option value='" + encodedId + "'>" + g_stationConfig.stopNames[i] + "   ("+g_stationConfig.stopDistances[i]+"m)</option>";
  }
  stopsHtml += "<option value='__manual__'>Manuell eingeben...</option>";
  if (g_stationConfig.stopNames.size() == 0) stopsHtml = "<option>Keine Haltestellen gefunden</option>";
  page.replace("{{STOPS}}", stopsHtml);

  // Replace city, ssid, etc. with RTC config data
  page.replace("{{CITY}}", config.cityName);
  page.replace("{{ROUTER}}", config.ssid);
  page.replace("{{IP}}", config.ipAddress);
  page.replace("{{MDNS}}", ".local"); // mDNS hostname
  
  // Replace configuration values with current RTC settings
  page.replace("{{WEATHER_INTERVAL}}", String(config.weatherInterval));
  page.replace("{{TRANSPORT_INTERVAL}}", String(config.transportInterval));
  page.replace("{{TRANSPORT_ACTIVE_START}}", config.transportActiveStart);
  page.replace("{{TRANSPORT_ACTIVE_END}}", config.transportActiveEnd);
  page.replace("{{WALKING_TIME}}", String(config.walkingTime));
  page.replace("{{SLEEP_START}}", config.sleepStart);
  page.replace("{{SLEEP_END}}", config.sleepEnd);
  page.replace("{{WEEKEND_MODE}}", config.weekendMode ? "checked" : "");
  page.replace("{{WEEKEND_TRANSPORT_START}}", config.weekendTransportStart);
  page.replace("{{WEEKEND_TRANSPORT_END}}", config.weekendTransportEnd);
  page.replace("{{WEEKEND_SLEEP_START}}", config.weekendSleepStart);
  page.replace("{{WEEKEND_SLEEP_END}}", config.weekendSleepEnd);
  
  // Build JavaScript array for saved filters from RTC config
  String filtersJs = "[";
  std::vector<String> activeFilters = ConfigManager::getActiveFilters();
  for (size_t i = 0; i < activeFilters.size(); i++) {
    if (i > 0) filtersJs += ",";
    filtersJs += "\"" + activeFilters[i] + "\"";
  }
  filtersJs += "]";
  page.replace("{{SAVED_FILTERS}}", filtersJs);

  server.send(200, "text/html; charset=utf-8", page);
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer &server) {
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

    // Update configuration via ConfigManager (saves to RTC memory)
    if (doc.containsKey("city") && doc.containsKey("cityLat") && doc.containsKey("cityLon")) {
        ConfigManager::setLocation(doc["cityLat"].as<float>(), doc["cityLon"].as<float>(), doc["city"].as<String>());
    }
    if (doc.containsKey("stopId") && doc.containsKey("stopName")) {
        ConfigManager::setStop(doc["stopId"].as<String>(), doc["stopName"].as<String>());
    }
    
    // Update timing configuration
    if (doc.containsKey("weatherInterval") || doc.containsKey("transportInterval") || doc.containsKey("walkingTime")) {
        int weatherInt = doc.containsKey("weatherInterval") ? doc["weatherInterval"].as<int>() : ConfigManager::getConfig().weatherInterval;
        int transportInt = doc.containsKey("transportInterval") ? doc["transportInterval"].as<int>() : ConfigManager::getConfig().transportInterval;
        int walkTime = doc.containsKey("walkingTime") ? doc["walkingTime"].as<int>() : ConfigManager::getConfig().walkingTime;
        ConfigManager::setTimingConfig(weatherInt, transportInt, walkTime);
    }
    
    // Update active hours
    if (doc.containsKey("transportActiveStart") && doc.containsKey("transportActiveEnd")) {
        ConfigManager::setActiveHours(doc["transportActiveStart"].as<String>(), doc["transportActiveEnd"].as<String>());
    }
    
    // Update sleep hours
    if (doc.containsKey("sleepStart") && doc.containsKey("sleepEnd")) {
        ConfigManager::setSleepHours(doc["sleepStart"].as<String>(), doc["sleepEnd"].as<String>());
    }
    
    // Update weekend mode
    if (doc.containsKey("weekendMode")) {
        ConfigManager::setWeekendMode(doc["weekendMode"].as<bool>());
    }
    
    // Update weekend hours
    if (doc.containsKey("weekendTransportStart") && doc.containsKey("weekendTransportEnd") && 
        doc.containsKey("weekendSleepStart") && doc.containsKey("weekendSleepEnd")) {
        ConfigManager::setWeekendHours(
            doc["weekendTransportStart"].as<String>(), doc["weekendTransportEnd"].as<String>(),
            doc["weekendSleepStart"].as<String>(), doc["weekendSleepEnd"].as<String>()
        );
    }
    
    // Update ÖPNV filters
    if (doc.containsKey("filters")) {
        std::vector<String> filters;
        JsonArray filterArray = doc["filters"];
        for (JsonVariant v : filterArray) {
            filters.push_back(v.as<String>());
        }
        ConfigManager::setActiveFilters(filters);
    }
    
    // Also update the global config for web interface compatibility
    if (doc.containsKey("city")) g_stationConfig.cityName = doc["city"].as<String>();
    if (doc.containsKey("cityLat")) g_stationConfig.latitude = doc["cityLat"].as<float>();
    if (doc.containsKey("cityLon")) g_stationConfig.longitude = doc["cityLon"].as<float>();
    if (doc.containsKey("stopId")) g_stationConfig.selectedStopId = doc["stopId"].as<String>();
    if (doc.containsKey("stopName")) g_stationConfig.selectedStopName = doc["stopName"].as<String>();
    
    // Mark configuration as valid and ready for operational mode
    RTCConfigData& config = ConfigManager::getConfig();
    config.isValid = true;
    
    // Save to NVS (saves RTC memory to persistent storage)
    if (!configMgr.saveToNVS()) {
        server.send(500, "text/plain", "Failed to save config to NVS");
        return;
    }
    
    // Set configuration mode to false (operational mode)
    ConfigManager::setConfigMode(false);
    
    ESP_LOGI("CONFIG", "Configuration saved successfully. Switching to operational mode.");
    
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
