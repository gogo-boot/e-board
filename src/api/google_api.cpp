#include "api/google_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_log.h>
#include "sec/aes_crypto.h"

static const char* TAG = "GOOGLE_API";

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
    ESP_LOGD(TAG, "WiFi JSON: %s", output.c_str());
    return output;
}

bool getLocationFromGoogle(float& lat, float& lon) {
    String wifiJson = buildWifiJson();
    HTTPClient http;

    // Use static utility method for secure API key decryption (no caching)
    String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + String(
        AESCrypto::getGoogleAPIKey().c_str());

    ESP_LOGI(TAG, "Requesting location from Google: %s", url.c_str());
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(wifiJson);
    if (httpCode > 0) {
        String payload = http.getString();
        ESP_LOGD(TAG, "Google geolocation response: %s", payload.c_str());
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            lat = doc["location"]["lat"];
            lon = doc["location"]["lng"];
            ESP_LOGI(TAG, "Location found: lat=%.6f, lon=%.6f", lat, lon);
            http.end();
            return true;
        } else {
            ESP_LOGE(TAG, "Failed to parse Google geolocation JSON: %s", error.c_str());
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed, error: %s", http.errorToString(httpCode).c_str());
    }
    http.end();
    return false;
}
