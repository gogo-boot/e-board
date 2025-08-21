#include "api/rmv_api.h"
#include "api/rmv_json_parser.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>
#include "secrets/rmv_secrets.h"
#include "util/util.h"
#include <esp_log.h>
#include <StreamUtils.h>
#include "config/config_struct.h"
#include "config/config_manager.h"

static const char* TAG = "RMV_API";
// const size_t JSON_CAPACITY = 16384; // 16KB - safer for API responses
const size_t JSON_CAPACITY = 10240; // 10KB - safer for API responses

namespace {
    StaticJsonDocument<256> departureFilter;

    void initDepartureFilter() {
        departureFilter["Departure"][0]["name"] = true;
        departureFilter["Departure"][0]["time"] = true;
        departureFilter["Departure"][0]["track"] = true;
        departureFilter["Departure"][0]["rtTime"] = true;
        departureFilter["Departure"][0]["direction"] = true;
        departureFilter["Departure"][0]["directionFlag"] = true;

        departureFilter["Departure"][0]["Product"][0]["catOut"] = true;
        departureFilter["Departure"][0]["Messages"]["Message"][0]["head"] = true;
        departureFilter["Departure"][0]["Messages"]["Message"][0]["lead"] = true;
        departureFilter["Departure"][0]["Messages"]["Message"][0]["text"] = true;
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

void getNearbyStops(float lat, float lon) {
    Util::printFreeHeap("Before RMV request:");
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
            g_webConfigPageData.stopIds.clear();
            g_webConfigPageData.stopNames.clear();
            g_webConfigPageData.stopDistances.clear();
            JsonArray stops = doc["stopLocationOrCoordLocation"];
            // extern ConfigOption g_webConfigPageData;
            for (JsonObject item : stops) {
                JsonObject stop = item["StopLocation"];
                if (!stop.isNull()) {
                    const char* id = stop["id"] | "";
                    const char* name = stop["name"] | "";
                    float lon = stop["lon"] | 0.0;
                    float lat = stop["lat"] | 0.0;
                    int dist = stop["dist"] | 0;
                    int products = stop["products"] | 0;
                    String type = (products & 64) ? "train" : "bus"; // Example: RMV uses bitmask for products
                    stations.push_back({String(id), String(name), type});
                    g_webConfigPageData.stopIds.push_back(id);
                    g_webConfigPageData.stopNames.push_back(name);
                    g_webConfigPageData.stopDistances.push_back(dist);
                    ESP_LOGI(TAG, "Stop ID: %s, Name: %s, Lon: %f, Lat: %f, Type: %s", id, name, lon, lat,
                             type.c_str());
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

bool getDepartureFromRMV(const char* stopId, DepartureData& departData) {
    ESP_LOGI(TAG, "Fetching departure data for stop: %s", stopId);

    HTTPClient http;
    String encodedId = Util::urlEncode(String(stopId));
    String url = "https://www.rmv.de/hapi/departureBoard?accessId=" + String(RMV_API_KEY) +
        "&id=" + encodedId +
        "&format=json&maxJourneys=30";

    String urlForLog = url;
    int keyPos = urlForLog.indexOf("accessId=");
    if (keyPos != -1) {
        int keyEnd = urlForLog.indexOf('&', keyPos);
        if (keyEnd == -1) keyEnd = urlForLog.length();
        urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
    }

    ESP_LOGI(TAG, "Requesting departure board: %s", urlForLog.c_str());
    http.begin(url);

    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    initDepartureFilter();

    // Create the raw and decoded stream
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());

    // Choose the stream based on the Transfer-Encoding header
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    DynamicJsonDocument doc(JSON_CAPACITY);
    DeserializationOption::NestingLimit nestingLimit(20);
    // ReadLoggingStream loggingStream(http.getStream(), Serial);

    // ReadBufferingStream bufferingStream(http.getStream(), 64);

    // Always check for memory errors
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(departureFilter),
                                                 nestingLimit);

    if (error) {
        ESP_LOGE(TAG, "JSON parse failed: %s", error.c_str());
        if (error == DeserializationError::NoMemory) {
            ESP_LOGE(TAG, "Increase JSON_CAPACITY (current: %d)", JSON_CAPACITY);
        }
        return false;
    }

    http.end();

    // Pretty print to string
    String prettyJson;
    serializeJsonPretty(doc, prettyJson);
    ESP_LOGI(TAG, "JSON Document (pretty):\n%s", prettyJson.c_str());

    // Check actual memory usage
    // Memory used: 7128/10240 bytes for 30 rmv departure response
    ESP_LOGI(TAG, "Memory used: %u/%u bytes", doc.memoryUsage(), doc.capacity());

    // Check heap before/after
    ESP_LOGI(TAG, "Free heap: %u bytes", ESP.getFreeHeap());
    // ESP_LOGI(TAG, "Received RMV response with length: %d bytes", payload.length());

    // Set basic departure data
    departData.stopId = String(stopId);

    return true;
}
