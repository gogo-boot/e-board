#include "config/config_page.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include "config/config_manager.h"
#include "config/config_page_data.h"
#include "util/util.h"
#include "sec/aes_crypto.h"
#include "util/sleep_utils.h"
#include "global_instances.h"

static const char* TAG = "CONFIG";

void handleStationSelect(WebServer& server) {
    File file = LittleFS.open("/station_select.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Template not found");
        return;
    }
    String page = file.readString();
    file.close();

    ConfigPageData& pageData = ConfigPageData::getInstance();
    String html;
    for (size_t i = 0; i < pageData.getStopCount(); ++i) {
        html += "<div class='station'>";
        html += "<input type='radio' name='station' value='" + pageData.getStopId(i) + "'>";
        html += pageData.getStopName(i) + " (ID: " + pageData.getStopId(i) + ")";
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

    ConfigPageData& pageData = ConfigPageData::getInstance();
    // Replace reserved keywords
    page.replace("{{LAT}}", String(pageData.getLatitude(), 6));
    page.replace("{{LON}}", String(pageData.getLongitude(), 6));

    // Build <option> list for stops, add manual entry option
    String stopsHtml = "<option value=''>Bitte wählen...</option>";
    for (size_t i = 0; i < pageData.getStopCount(); ++i) {
        String encodedId = Util::urlEncode(pageData.getStopId(i));
        stopsHtml += "<option value='" + encodedId + "'>" + pageData.getStopName(i) + "   (" +
            pageData.getStopDistance(i) + "m)</option>";
    }
    stopsHtml += "<option value='__manual__'>Manuell eingeben...</option>";
    if (pageData.getStopCount() == 0) stopsHtml = "<option>Keine Haltestellen gefunden</option>";
    page.replace("{{STOPS}}", stopsHtml);

    // Replace city, ssid, etc.
    page.replace("{{CITY}}", pageData.getCityName());

    // Get configuration from ConfigManager
    RTCConfigData& config = ConfigManager::getConfig();
    page.replace("{{ROUTER}}", config.ssid);
    page.replace("{{IP}}", pageData.getIPAddress());
    page.replace("{{MDNS}}", "mystation.local");

    // Replace configuration values with current settings from ConfigManager
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
    // Replace OTA configuration values
    page.replace("{{OTA_ENABLED}}", config.otaEnabled ? "true" : "false");
    page.replace("{{OTA_CHECK_TIME}}", config.otaCheckTime);

    // Build JavaScript array for saved filters from ConfigManager
    std::vector<String> activeFilters = ConfigManager::getActiveFilters();
    String filtersJs = "[";
    for (size_t i = 0; i < activeFilters.size(); i++) {
        if (i > 0) filtersJs += ",";
        filtersJs += "\"" + activeFilters[i] + "\"";
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
        std::vector<String> filterList;
        JsonArray filters = doc["filters"];
        for (JsonVariant v : filters) {
            filterList.push_back(v.as<String>());
        }
        ConfigManager::setActiveFilters(filterList);
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

    // Handle OTA configuration
    if (doc.containsKey("otaEnabled")) config.otaEnabled = doc["otaEnabled"].as<bool>();
    if (doc.containsKey("otaCheckTime"))
        strncpy(config.otaCheckTime, doc["otaCheckTime"].as<const char*>(),
                sizeof(config.otaCheckTime) - 1);

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

    /*
    *The deep sleep after saving configuration serves several purposes:
    Apply new settings cleanly: After saving configuration changes (WiFi credentials, station settings, schedules, etc.), the device needs to restart to properly initialize with the new configuration.
    Mode transition: The code switches from AP (Access Point) mode to STA (Station) mode on line 209. Deep sleep followed by wake-up ensures a clean transition and proper connection to the configured WiFi network.
    Reset state: Deep sleep is a common pattern in ESP32 development to reset the device state and start fresh with the updated configuration from NVS (Non-Volatile Storage).
    The 1-second sleep is intentionally brief - just long enough to ensure the HTTP response is sent to the client before the device resets. When it wakes up, it will load the newly saved configuration from NVS and operate in normal mode rather than configuration mode.
    */
    enterDeepSleep(1); // Enter deep sleep for 1 seconds
}

// AJAX handler to search cities by postal code (GET /api/city?q=...)
void handleCityAutocomplete(WebServer& server) {
    String query = server.hasArg("q") ? server.arg("q") : "";

    // Validate postal code: must be 5 digits
    if (query.length() < 5) {
        server.send(200, "application/json", "[]");
        return;
    }

    // Validate that it's numeric
    for (size_t i = 0; i < query.length(); i++) {
        if (!isdigit(query.charAt(i))) {
            server.send(200, "application/json", "[]");
            return;
        }
    }

    ESP_LOGI(TAG, "Postal code search query: %s", query.c_str());

    HTTPClient http;
    String url = "https://nominatim.openstreetmap.org/search?postalcode=" + Util::urlEncode(query) +
        "&format=json&limit=5&addressdetails=1&countrycodes=de";

    http.begin(url);
    http.addHeader("User-Agent", "ESP32-MyStation/1.0");

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        ESP_LOGD(TAG, "Nominatim response: %s", payload.c_str());

        // Parse the response and extract city, lat, lon
        DynamicJsonDocument docIn(4096);
        DeserializationError error = deserializeJson(docIn, payload);

        if (!error) {
            DynamicJsonDocument docOut(2048);
            JsonArray outArray = docOut.to<JsonArray>();

            JsonArray results = docIn.as<JsonArray>();
            for (JsonObject result : results) {
                JsonObject city = outArray.createNestedObject();

                // Get display name or city name
                String displayName = result["display_name"].as<String>();
                String lat = result["lat"].as<String>();
                String lon = result["lon"].as<String>();
                // Try to extract city name from address
                String cityName = displayName;
                if (result.containsKey("address")) {
                    JsonObject addr = result["address"];
                    if (addr.containsKey("city")) {
                        cityName = addr["city"].as<String>();
                    } else if (addr.containsKey("town")) {
                        cityName = addr["town"].as<String>();
                    } else if (addr.containsKey("village")) {
                        cityName = addr["village"].as<String>();
                    }
                }

                city["name"] = cityName;
                city["display"] = displayName;
                city["lat"] = lat;
                city["lon"] = lon;
            }

            String out;
            serializeJson(docOut, out);
            http.end();
            server.send(200, "application/json", out);
            return;
        } else {
            ESP_LOGE(TAG, "Failed to parse Nominatim JSON: %s", error.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Nominatim API failed: %s", http.errorToString(httpCode).c_str());
    }

    http.end();
    server.send(200, "application/json", "[]");
}

// AJAX handler to resolve station/stop name (GET /api/stop?q=...)
void handleStopAutocomplete(WebServer& server) {
    String query = server.hasArg("q") ? server.arg("q") : "";

    if (query.length() < 3) {
        server.send(200, "application/json", "[]");
        return;
    }

    ESP_LOGI(TAG, "Station search query: %s", query.c_str());

    // Use static utility method for secure API key decryption
    std::string decrypted = AESCrypto::getRMVAPIKey();

    HTTPClient http;
    String url = "https://www.rmv.de/hapi/location.name?accessId=" + String(decrypted.c_str()) +
        "&input=" + Util::urlEncode(query) +
        "&format=json&maxNo=5";

    String urlForLog = url;
    int keyPos = urlForLog.indexOf("accessId=");
    if (keyPos != -1) {
        int keyEnd = urlForLog.indexOf('&', keyPos);
        if (keyEnd == -1) keyEnd = urlForLog.length();
        urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
    }

    ESP_LOGI(TAG, "Requesting RMV location search: %s", urlForLog.c_str());

    http.begin(url);

    // Collect Transfer-Encoding header to handle chunked responses
    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "RMV API failed: %s", http.errorToString(httpCode).c_str());
        http.end();
        server.send(200, "application/json", "[]");
        return;
    }

    // Create JSON filter to only parse "id" and "name" fields
    StaticJsonDocument<128> stationFilter;
    stationFilter["stopLocationOrCoordLocation"][0]["StopLocation"]["id"] = true;
    stationFilter["stopLocationOrCoordLocation"][0]["StopLocation"]["name"] = true;

    // Create the raw and decoded stream
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());

    // Choose the stream based on the Transfer-Encoding header
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    // Use smaller JSON document since we're only extracting id and name
    DynamicJsonDocument docIn(2048); // Reduced from 4096
    DeserializationOption::NestingLimit nestingLimit(15);

    // Parse with filter to minimize memory usage
    DeserializationError error = deserializeJson(docIn, response,
                                                 DeserializationOption::Filter(stationFilter),
                                                 nestingLimit);

    if (error) {
        ESP_LOGE(TAG, "Failed to parse RMV JSON: %s", error.c_str());
        if (error == DeserializationError::NoMemory) {
            ESP_LOGE(TAG, "Increase JSON capacity or reduce maxNo parameter");
        }
        http.end();
        server.send(200, "application/json", "[]");
        return;
    }

    http.end();

    // Build compact output with only id and name
    DynamicJsonDocument docOut(1024); // Reduced from 2048
    JsonArray outArray = docOut.to<JsonArray>();

    JsonArray locations = docIn["stopLocationOrCoordLocation"];
    for (JsonVariantConst item : locations) {
        JsonObjectConst stopLoc = item["StopLocation"];
        if (!stopLoc.isNull()) {
            JsonObject stop = outArray.createNestedObject();
            stop["name"] = stopLoc["name"].as<String>();
            stop["id"] = stopLoc["id"].as<String>();
        }
    }

    String out;
    serializeJson(docOut, out);
    ESP_LOGI(TAG, "Found %d stations", outArray.size());
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

void handleCityAutocompleteWrapper() {
    handleCityAutocomplete(*g_server);
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
            delay(500);
        }
    }
    ESP_LOGI(TAG, "Setting up web server...");
    g_server = &server;
    server.on("/", handleConfigPageWrapper);
    server.on("/save_config", HTTP_POST, handleSaveConfigWrapper);
    server.on("/api/city", HTTP_GET, handleCityAutocompleteWrapper);
    server.on("/api/stop", HTTP_GET, handleStopAutocompleteWrapper);
    server.begin();
    ESP_LOGI("WEB_SERVER", "HTTP server started.");
}
