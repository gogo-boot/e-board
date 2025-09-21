#include "api/rmv_api.h"
#include <HTTPClient.h>
#include <vector>
#include <Arduino.h>
#include "secrets/rmv_secrets.h"
#include "util/util.h"
#include <esp_log.h>
#include <StreamUtils.h>
#include "config/config_struct.h"

static const char* TAG = "RMV_API";
// const size_t JSON_CAPACITY = 16384; // 16KB - safer for API responses
const size_t JSON_CAPACITY = 10240; // 10KB - safer for API responses

namespace {
    StaticJsonDocument<256> departureFilter;

    void initDepartureFilter() {
        departureFilter["Departure"][0]["time"] = true;
        departureFilter["Departure"][0]["track"] = true;
        departureFilter["Departure"][0]["rtTime"] = true;
        departureFilter["Departure"][0]["cancelled"] = true;
        departureFilter["Departure"][0]["direction"] = true;
        departureFilter["Departure"][0]["directionFlag"] = true;

        departureFilter["Departure"][0]["Product"][0]["line"] = true;
        departureFilter["Departure"][0]["Product"][0]["catOut"] = true;
        departureFilter["Departure"][0]["Messages"]["Message"][0]["head"] = true;
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

// Populate departure data from JSON document
bool populateDepartureData(const DynamicJsonDocument& doc, DepartureData& departData) {
    ESP_LOGI(TAG, "Populating departure data from JSON response");

    // Clear existing departures
    departData.departures.clear();
    departData.departureCount = 0;

    // Use JsonArrayConst for const document
    JsonArrayConst departures = doc["Departure"];
    if (departures.size() == 0) {
        ESP_LOGW(TAG, "Departure array is empty");
        return false; // ← Also return false for empty array
    }
    ESP_LOGI(TAG, "Found %d departures in response", departures.size());

    // Reserve capacity to avoid multiple reallocations
    departData.departures.reserve(departures.size());

    for (JsonVariantConst departureVariant : departures) {
        DepartureInfo depInfo;

        /*
        {
          "Departure": [
            {
              "Product": [
                {
                  "line": "S6",
                  "catOut": "Bus"
                }
              ],
              "Messages": {
                "Message": [
                  {
                    "head": "S3, S4, S5: nächtliche Teilausfälle mit Ersatzverkehr"
                  }
                ]
              },
              "time": "22:37:00",
              "rtTime": "22:37:00",
              "cancelled": true,
              "direction": "Frankfurt (Main) Hauptbahnhof Südseite",
              "directionFlag": "1"
            }
        */
        // Extract basic departure information with safe string conversion
        const char* direction = departureVariant["direction"];
        const char* directionFlag = departureVariant["directionFlag"];
        const char* time = departureVariant["time"];
        const char* rtTime = departureVariant["rtTime"];
        const bool cancelled = departureVariant["cancelled"].as<bool>();
        const char* track = departureVariant["track"];

        // Safe string assignment with null checks
        depInfo.direction = direction ? String(direction) : "";
        depInfo.directionFlag = directionFlag ? String(directionFlag) : "";
        depInfo.time = time ? String(time) : "";
        depInfo.rtTime = rtTime ? String(rtTime) : "";
        depInfo.cancelled = cancelled ? cancelled : false;
        depInfo.track = track ? String(track) : "";

        // Extract category from Product array with safety checks
        if (departureVariant["Product"].size() > 0) {
            // Direct path access
            const char* catOut = departureVariant["Product"][0]["catOut"];
            const char* line = departureVariant["Product"][0]["line"];

            depInfo.category = catOut ? String(catOut) : "";
            depInfo.line = line ? String(line) : "";
        }

        // Extract category from Product array with safety checks
        if (departureVariant["Messages"]["Message"].size() > 0) {
            // Direct path access
            const char* head = departureVariant["Messages"]["Message"][0]["head"];
            depInfo.text = head ? String(head) : "";
        }

        // Add to departures vector
        departData.departures.push_back(depInfo);

        ESP_LOGD(TAG, "Added: %s -> %s at %s (RT: %s) [%s]",
                 depInfo.line.c_str(),
                 depInfo.direction.c_str(),
                 depInfo.time.c_str(),
                 depInfo.rtTime.c_str(),
                 depInfo.category.c_str());
    }

    departData.departureCount = static_cast<int>(departData.departures.size());
    ESP_LOGI(TAG, "Successfully populated %d departures", departData.departureCount);

    return true;
}

bool getDepartureFromRMV(const char* stopId, DepartureData& departData) {
    ESP_LOGI(TAG, "Fetching departure data for stop: %s", stopId);

    HTTPClient http;
    String encodedId = Util::urlEncode(String(stopId));
    String url = "https://www.rmv.de/hapi/departureBoard?accessId=" + String(RMV_API_KEY) +
        "&id=" + encodedId +
        "&format=json&maxJourneys=20&products=8";

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
    ESP_LOGD(TAG, "JSON Document (pretty):\n%s", prettyJson.c_str());

    // Check actual memory usage
    // Memory used: 7128/10240 bytes for 30 rmv departure response
    ESP_LOGI(TAG, "Memory used: %u/%u bytes", doc.memoryUsage(), doc.capacity());

    // Check heap before/after
    ESP_LOGI(TAG, "Free heap: %u bytes", ESP.getFreeHeap());

    // Set basic departure data
    departData.stopId = String(stopId);

    // Populate departure data from JSON
    if (!populateDepartureData(doc, departData)) {
        ESP_LOGE(TAG, "Failed to populate departure data");
        return false;
    }

    return true;
}
