#include "ota/ota_updater.h"
#include <esp_log.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* TAG = "OTA";
const char* CURRENT_VERSION = "1.0.0"; // Define your current version

// GitHub API endpoint for latest release
// const char* GITHUB_API_URL = "https://api.github.com/repos/gogo-boot/e-board/releases/latest";
const char* GITHUB_API_URL =
    "https://raw.githubusercontent.com/gogo-boot/e-board/refs/heads/main/test/wifi/wifi_info.json";

bool OTAUpdater::checkForUpdate(FirmwareInfo& info) {
    HTTPClient http;
    http.begin(GITHUB_API_URL);
    http.addHeader("User-Agent", "ESP32-OTA-Updater");

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Failed to check for updates: %d", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse JSON response
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    String latestVersion = doc["considerIp"].as<String>();
    if (latestVersion.isEmpty()) {
        ESP_LOGE(TAG, "Invalid response from GitHub API");
        return false;
    }

    // Check if update is available
    if (latestVersion != getCurrentVersion()) {
        info.version = latestVersion;
        info.available = true;

        // Find the correct firmware asset for your board
        JsonArray assets = doc["assets"];
        for (JsonVariant asset : assets) {
            String name = asset["name"].as<String>();

            // Match firmware file for your specific board
            if (name.indexOf("esp32-c3") != -1 && name.endsWith(".bin")) {
                info.downloadUrl = asset["browser_download_url"].as<String>();
                info.size = asset["size"].as<int>();
                ESP_LOGI(TAG, "Update available: %s -> %s", getCurrentVersion().c_str(), latestVersion.c_str());
                return true;
            }
        }
    }

    info.available = false;
    return true;
}

bool OTAUpdater::performUpdate(const String& url) {
    ESP_LOGI(TAG, "Starting OTA update from: %s", url.c_str());

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %d", httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        ESP_LOGE(TAG, "Invalid content length");
        http.end();
        return false;
    }

    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        ESP_LOGE(TAG, "Not enough space for OTA update");
        http.end();
        return false;
    }

    WiFiClient* client = http.getStreamPtr();
    size_t written = 0;
    uint8_t buffer[128];

    ESP_LOGI(TAG, "Starting firmware download...");

    while (http.connected() && written < contentLength) {
        size_t available = client->available();
        if (available) {
            int readBytes = client->readBytes(buffer, min(available, sizeof(buffer)));
            written += Update.write(buffer, readBytes);

            // Show progress
            int progress = (written * 100) / contentLength;
            static int lastProgress = -1;
            if (progress != lastProgress && progress % 10 == 0) {
                ESP_LOGI(TAG, "Progress: %d%%", progress);
                lastProgress = progress;
            }
        }
        delay(1);
    }

    http.end();

    if (Update.end()) {
        ESP_LOGI(TAG, "OTA update successful! Rebooting...");
        ESP.restart();
        return true;
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", Update.errorString());
        return false;
    }
}

String OTAUpdater::getCurrentVersion() {
    return String(CURRENT_VERSION);
}
