#include "config/config_page.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "util/util.h"
#include <esp_log.h>
#include "util/sleep_utils.h"

static const char* TAG = "CONFIG";

extern float g_lat, g_lon;

extern ConfigOption g_webConfigPageData;

void handleStationSelect(WebServer& server) {
    File file = LittleFS.open("/station_select.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Template not found");
        return;
    }
    String page = file.readString();
    file.close();
    String html;
    for (size_t i = 0; i < g_webConfigPageData.stopNames.size(); ++i) {
        html += "<div class='station'>";
        html += "<input type='radio' name='station' value='" + g_webConfigPageData.stopIds[i] + "'>";
        html += g_webConfigPageData.stopNames[i] + " (ID: " + g_webConfigPageData.stopIds[i] + ")";
        html += "</div>";
    }
    page.replace("{{stations}}", html);
    server.send(200, "text/html", page);
}

// Update the config page handler to serve config_my_station.html
void handleConfigPage(WebServer& server) {
    File file = LittleFS.open("/config_my_station.html", "r");
    if (!file) {
        server.send(404, "text/plain", "Konfigurationsseite nicht gefunden");
        return;
    }
    String page = file.readString();
    file.close();

    // Replace reserved keywords
    page.replace("{{LAT}}", String(g_webConfigPageData.latitude, 6));
    page.replace("{{LON}}", String(g_webConfigPageData.longitude, 6));

    // Build <option> list for stops, add manual entry option
    String stopsHtml = "<option value=''>Bitte wählen...</option>";
    for (size_t i = 0; i < g_webConfigPageData.stopNames.size(); ++i) {
        String encodedId = Util::urlEncode(g_webConfigPageData.stopIds[i]);
        stopsHtml += "<option value='" + encodedId + "'>" + g_webConfigPageData.stopNames[i] + "   (" +
            g_webConfigPageData.stopDistances[i] + "m)</option>";
    }
    stopsHtml += "<option value='__manual__'>Manuell eingeben...</option>";
    if (g_webConfigPageData.stopNames.size() == 0) stopsHtml = "<option>Keine Haltestellen gefunden</option>";
    page.replace("{{STOPS}}", stopsHtml);

    // Replace city, ssid, etc.
    page.replace("{{CITY}}", g_webConfigPageData.cityName);
    // Separate Router (SSID) and IP info
    page.replace("{{ROUTER}}", g_webConfigPageData.ssid);
    page.replace("{{IP}}", g_webConfigPageData.ipAddress); // Replace with IP info if available
    page.replace("{{MDNS}}", ".local"); // mDNS hostname

    // Replace configuration values with current settings
    page.replace("{{WEATHER_INTERVAL}}", String(g_webConfigPageData.weatherInterval));
    page.replace("{{TRANSPORT_INTERVAL}}", String(g_webConfigPageData.transportInterval));
    page.replace("{{TRANSPORT_ACTIVE_START}}", g_webConfigPageData.transportActiveStart);
    page.replace("{{TRANSPORT_ACTIVE_END}}", g_webConfigPageData.transportActiveEnd);
    page.replace("{{WALKING_TIME}}", String(g_webConfigPageData.walkingTime));
    page.replace("{{SLEEP_START}}", g_webConfigPageData.sleepStart);
    page.replace("{{SLEEP_END}}", g_webConfigPageData.sleepEnd);
    page.replace("{{WEEKEND_MODE}}", g_webConfigPageData.weekendMode ? "checked" : "");
    page.replace("{{WEEKEND_TRANSPORT_START}}", g_webConfigPageData.weekendTransportStart);
    page.replace("{{WEEKEND_TRANSPORT_END}}", g_webConfigPageData.weekendTransportEnd);
    page.replace("{{WEEKEND_SLEEP_START}}", g_webConfigPageData.weekendSleepStart);
    page.replace("{{WEEKEND_SLEEP_END}}", g_webConfigPageData.weekendSleepEnd);

    // Build JavaScript array for saved filters
    String filtersJs = "[";
    for (size_t i = 0; i < g_webConfigPageData.oepnvFilters.size(); i++) {
        if (i > 0) filtersJs += ",";
        filtersJs += "\"" + g_webConfigPageData.oepnvFilters[i] + "\"";
    }
    filtersJs += "]";
    page.replace("{{SAVED_FILTERS}}", filtersJs);

    server.send(200, "text/html; charset=utf-8", page);
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer& server) {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    ConfigManager& configMgr = ConfigManager::getInstance();
    RTCConfigData& config = configMgr.getConfig();

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
    if (doc.containsKey("city")) strncpy(config.cityName, doc["city"].as<const char*>(), sizeof(config.cityName) - 1);
    if (doc.containsKey("cityLat")) config.latitude = doc["cityLat"].as<float>();
    if (doc.containsKey("cityLon")) config.longitude = doc["cityLon"].as<float>();
    if (doc.containsKey("stopId"))
        strncpy(config.selectedStopId, doc["stopId"].as<const char*>(),
                sizeof(config.selectedStopId) - 1);
    if (doc.containsKey("stopName"))
        strncpy(config.selectedStopName, doc["stopName"].as<const char*>(),
                sizeof(config.selectedStopName) - 1);

    // Update ÖPNV filters
    if (doc.containsKey("filters")) {
        g_webConfigPageData.oepnvFilters.clear();
        JsonArray filters = doc["filters"];
        for (JsonVariant v : filters) {
            g_webConfigPageData.oepnvFilters.push_back(v.as<String>());
        }
    }

    // Update new configuration values
    if (doc.containsKey("weatherInterval")) config.weatherInterval = doc["weatherInterval"].as<int>();
    if (doc.containsKey("transportInterval")) config.transportInterval = doc["transportInterval"].as<int>();

    // NEW: Handle display mode configuration
    if (doc.containsKey("displayMode")) {
        config.displayMode = doc["displayMode"].as<uint8_t>();
        ESP_LOGI(TAG, "Display mode set to: %d", config.displayMode);
    }

    if (doc.containsKey("transportActiveStart"))
        strncpy(config.transportActiveStart,
                doc["transportActiveStart"].as<const char*>(),
                sizeof(config.transportActiveStart) - 1);
    if (doc.containsKey("transportActiveEnd"))
        strncpy(config.transportActiveEnd,
                doc["transportActiveEnd"].as<const char*>(),
                sizeof(config.transportActiveEnd) - 1);
    if (doc.containsKey("walkingTime")) config.walkingTime = doc["walkingTime"].as<int>();
    if (doc.containsKey("sleepStart"))
        strncpy(config.sleepStart, doc["sleepStart"].as<const char*>(),
                sizeof(config.sleepStart) - 1);
    if (doc.containsKey("sleepEnd"))
        strncpy(config.sleepEnd, doc["sleepEnd"].as<const char*>(),
                sizeof(config.sleepEnd) - 1);
    if (doc.containsKey("weekendMode")) config.weekendMode = doc["weekendMode"].as<bool>();
    if (doc.containsKey("weekendTransportStart"))
        strncpy(config.weekendTransportStart,
                doc["weekendTransportStart"].as<const char*>(),
                sizeof(config.weekendTransportStart) - 1);
    if (doc.containsKey("weekendTransportEnd"))
        strncpy(config.weekendTransportEnd,
                doc["weekendTransportEnd"].as<const char*>(),
                sizeof(config.weekendTransportEnd) - 1);
    if (doc.containsKey("weekendSleepStart"))
        strncpy(config.weekendSleepStart,
                doc["weekendSleepStart"].as<const char*>(),
                sizeof(config.weekendSleepStart) - 1);
    if (doc.containsKey("weekendSleepEnd"))
        strncpy(config.weekendSleepEnd, doc["weekendSleepEnd"].as<const char*>(),
                sizeof(config.weekendSleepEnd) - 1);

    // Save config mode to NVS (persists across power loss)
    configMgr.setConfigMode(false);

    // Save to NVS (and RTC memory automatically)
    if (!configMgr.saveToNVS()) {
        server.send(500, "text/plain", "Failed to save config to NVS");
        return;
    }

    // Switch to station mode only
#ifdef ESP32
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
#endif

    server.send(200, "application/json", "{\"status\":\"ok\"}");

    enterDeepSleep(1 * 1000000); // Enter deep sleep for 1 seconds
}

// AJAX handler to resolve station/stop name (GET /api/stop?q=...)
void handleStopAutocomplete(WebServer& server) {
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

// Global server reference for callback access
static WebServer* g_server = nullptr;

// Callback wrapper functions
void handleConfigPageWrapper() {
    handleConfigPage(*g_server);
}

void handleSaveConfigWrapper() {
    handleSaveConfig(*g_server);
}

void handleStopAutocompleteWrapper() {
    handleStopAutocomplete(*g_server);
}

void setupWebServer(WebServer& server) {
    // Initialize filesystem
    // It need to be done before load configurration html files
    if (!LittleFS.begin()) {
        ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
        while (true) {
            delay(1000);
        }
    }
    ESP_LOGI(TAG, "Setting up web server...");
    g_server = &server;
    server.on("/", handleConfigPageWrapper);
    server.on("/save_config", HTTP_POST, handleSaveConfigWrapper);
    server.on("/api/stop", HTTP_GET, handleStopAutocompleteWrapper);
    server.begin();
    ESP_LOGI("WEB_SERVER", "HTTP server started.");
}
