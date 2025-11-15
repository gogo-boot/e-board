#include "util/wifi_manager.h"
#include "config/config_manager.h"
#include "config/config_page_data.h"
#include "util/util.h"
#include <esp_log.h>

#include "config/config_struct.h"

static const char* TAG = "WIFI_MGR";

// RTC variables to persist WiFi state across deep sleep
RTC_DATA_ATTR static int cached_wifi_channel = 0;
RTC_DATA_ATTR static uint8_t cached_wifi_bssid[6] = {0};
RTC_DATA_ATTR static bool wifi_cache_valid = false;
RTC_DATA_ATTR static unsigned long wifi_cache_timestamp = 0;
RTC_DATA_ATTR static char cached_ssid[33] = {0}; // WiFi SSID max length is 32 + null terminator

// Cache validity duration: 24 hours (WiFi config rarely changes)
static const unsigned long CACHE_VALIDITY_MS = 24 * 60 * 60 * 1000UL;

void MyWiFiManager::reconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGD(TAG, "WiFi already connected: %s", WiFi.localIP().toString().c_str());
        return; // Already connected
    }

    ESP_LOGI(TAG, "WiFi disconnected, attempting to reconnect...");

    // Try fast reconnect first if we have cached WiFi state
    if (fastReconnectWiFi()) {
        ESP_LOGI(TAG, "Fast WiFi reconnect successful!");
        saveWiFiStateToRTC(); // Update cache timestamp
        return;
    }

    ESP_LOGI(TAG, "Fast reconnect failed, performing full WiFi scan...");

    // Fallback to full scan reconnection
    WiFi.begin(); // No parameters = use stored credentials with full scan

    int attempts = 0;
    const int max_attempts = FULL_CONNECT_TIMEOUT_MS / 500;

    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        ESP_LOGD(TAG, "Full scan connecting... attempt %d/%d", attempts + 1, max_attempts);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi reconnected successfully via full scan!");
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "Connected to SSID: %s", WiFi.SSID().c_str());

        // Save new WiFi state to RTC for future fast connects
        saveWiFiStateToRTC();
    } else {
        ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
        clearWiFiCache(); // Clear potentially stale cache
    }
}

void MyWiFiManager::setupWiFiAccessPointAndRestart(WiFiManager& wm) {
    ESP_LOGI(TAG, "Starting WiFi AP for Phase 1 configuration...");

    // Configure WiFiManager
    const char* menu[] = {"wifi"};
    wm.setMenu(menu, 1);
    wm.setConnectTimeout(20); // 20 seconds per connection attempt
    wm.setConnectRetries(3);
    wm.setMinimumSignalQuality(20); // Lower signal quality requirement
    wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    wm.setTitle("MyStation WiFi Setup");
    wm.setCountry("DE");

    // Start AP and wait for WiFi configuration
    String apName = Util::getUniqueSSID("MyStation");
    ESP_LOGI(TAG, "Starting AP with SSID: %s", apName.c_str());

    bool connected = wm.autoConnect(apName.c_str());

    ESP_LOGI(TAG, "WiFi connected to: %s", WiFi.SSID().c_str());
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());

    // Set success message after saving
    wm.setSaveConfigCallback([]() {
        ESP_LOGI(TAG, "wifi manager configuration is saved!");
        // Mark WiFi as configured
        RTCConfigData& config = ConfigManager::getConfig();
        config.wifiConfigured = true;

        // Update configuration with network info
        ConfigManager::setNetwork(WiFi.SSID(), WiFi.localIP().toString());

        // Save WiFi credentials to NVS
        ConfigManager& configMgr = ConfigManager::getInstance();
        ESP_LOGI(TAG, "Saving WiFi credentials to NVS:");
        ESP_LOGI(TAG, "  SSID: %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "  IP: %s", WiFi.localIP().toString().c_str());
        configMgr.saveToNVS();
        delay(100); // Give time for logs to be sent
        ESP.restart();
    });

    // Check internet connectivity
    if (hasInternetAccess()) {
        ESP_LOGW(TAG, "has internet access");
        RTCConfigData& config = ConfigManager::getConfig();
        config.wifiConfigured = true;

        // Update configuration with network info
        ConfigManager::setNetwork(WiFi.SSID(), WiFi.localIP().toString());

        // Save WiFi credentials to NVS
        ConfigManager& configMgr = ConfigManager::getInstance();
        ESP_LOGI(TAG, "Saving WiFi credentials to NVS:");
        ESP_LOGI(TAG, "  SSID: %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "  IP: %s", WiFi.localIP().toString().c_str());
        configMgr.saveToNVS();
        ESP.restart();
    }
}

bool MyWiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String MyWiFiManager::getLocalIP() {
    return WiFi.localIP().toString();
}

bool MyWiFiManager::fastReconnectWiFi() {
    if (!isWiFiStateCached()) {
        ESP_LOGD(TAG, "No valid WiFi state cached - cannot use fast reconnect");
        return false;
    }

    ESP_LOGI(TAG, "Attempting fast WiFi reconnect to cached network");
    ESP_LOGI(TAG, "Cached SSID: %s, Channel: %d", cached_ssid, cached_wifi_channel);

    // Use cached channel and BSSID for direct connection (skips scan)
    WiFi.begin(cached_ssid, nullptr, cached_wifi_channel, cached_wifi_bssid, true);

    unsigned long start_time = millis();
    int attempts = 0;
    const int max_attempts = FAST_CONNECT_TIMEOUT_MS / 500;

    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        attempts++;

        // Log progress every 2 seconds
        if (attempts % 4 == 0) {
            ESP_LOGD(TAG, "Fast connect attempt %d/%d (%.1fs)",
                     attempts, max_attempts, (millis() - start_time) / 1000.0);
        }
    }

    bool success = (WiFi.status() == WL_CONNECTED);
    unsigned long connect_time = millis() - start_time;

    if (success) {
        ESP_LOGI(TAG, "Fast WiFi connect successful in %lu ms", connect_time);
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
    } else {
        ESP_LOGW(TAG, "Fast WiFi connect failed after %lu ms", connect_time);
        // Don't clear cache yet - might be temporary network issue
    }

    return success;
}

void MyWiFiManager::saveWiFiStateToRTC() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(TAG, "Cannot save WiFi state - not connected");
        return;
    }

    // Get current WiFi connection info
    cached_wifi_channel = WiFi.channel();
    uint8_t* bssid = WiFi.BSSID();
    memcpy(cached_wifi_bssid, bssid, 6);
    strncpy(cached_ssid, WiFi.SSID().c_str(), sizeof(cached_ssid) - 1);
    cached_ssid[sizeof(cached_ssid) - 1] = '\0'; // Ensure null termination
    wifi_cache_timestamp = millis();
    wifi_cache_valid = true;

    ESP_LOGI(TAG, "Saved WiFi state to RTC memory:");
    ESP_LOGI(TAG, "  SSID: %s", cached_ssid);
    ESP_LOGI(TAG, "  Channel: %d", cached_wifi_channel);
    ESP_LOGI(TAG, "  BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
             cached_wifi_bssid[0], cached_wifi_bssid[1], cached_wifi_bssid[2],
             cached_wifi_bssid[3], cached_wifi_bssid[4], cached_wifi_bssid[5]);
}

bool MyWiFiManager::isWiFiStateCached() {
    if (!wifi_cache_valid) {
        ESP_LOGD(TAG, "WiFi cache marked invalid");
        return false;
    }

    if (cached_wifi_channel == 0 || strlen(cached_ssid) == 0) {
        ESP_LOGD(TAG, "WiFi cache incomplete (channel=%d, ssid_len=%d)",
                 cached_wifi_channel, strlen(cached_ssid));
        return false;
    }

    // Check if cache is still within validity period
    unsigned long current_time = millis();
    unsigned long cache_age;

    // Handle millis() overflow (every ~49 days)
    if (current_time >= wifi_cache_timestamp) {
        cache_age = current_time - wifi_cache_timestamp;
    } else {
        // millis() overflowed - invalidate cache for safety
        ESP_LOGW(TAG, "millis() overflow detected - invalidating WiFi cache");
        return false;
    }

    if (cache_age > CACHE_VALIDITY_MS) {
        ESP_LOGI(TAG, "WiFi cache expired (age: %.1f hours, limit: %.1f hours)",
                 cache_age / (1000.0 * 60.0 * 60.0), CACHE_VALIDITY_MS / (1000.0 * 60.0 * 60.0));
        return false;
    }

    ESP_LOGD(TAG, "WiFi cache valid (age: %.1f hours)",
             cache_age / (1000.0 * 60.0 * 60.0));
    return true;
}

void MyWiFiManager::clearWiFiCache() {
    wifi_cache_valid = false;
    cached_wifi_channel = 0;
    memset(cached_wifi_bssid, 0, 6);
    memset(cached_ssid, 0, sizeof(cached_ssid));
    wifi_cache_timestamp = 0;
    ESP_LOGI(TAG, "WiFi cache cleared");
}

bool MyWiFiManager::hasInternetAccess() {
    ESP_LOGI(TAG, "Checking internet connectivity...");

    // Method 1: DNS lookup test
    IPAddress dnsResult;
    if (WiFi.hostByName("www.google.com", dnsResult) != 1) {
        ESP_LOGW(TAG, "DNS lookup failed");
        return false;
    }
    ESP_LOGI(TAG, "DNS lookup successful: %s", dnsResult.toString().c_str());

    // Method 2: HTTP ping to reliable endpoint
    WiFiClient client;
    const char* testHost = "www.google.com";
    const int testPort = 80;

    ESP_LOGI(TAG, "Attempting HTTP connection to %s:%d", testHost, testPort);
    if (!client.connect(testHost, testPort, 5000)) {
        // 5 second timeout
        ESP_LOGW(TAG, "Connection to test host failed");
        return false;
    }

    // Send simple HTTP HEAD request
    client.println("HEAD / HTTP/1.1");
    client.print("Host: ");
    client.println(testHost);
    client.println("Connection: close");
    client.println();

    // Wait for response
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            ESP_LOGW(TAG, "HTTP request timeout");
            client.stop();
            return false;
        }
        delay(10);
    }

    // Read first line of response
    String line = client.readStringUntil('\r');
    client.stop();

    // Check for valid HTTP response
    if (line.indexOf("HTTP/1.") != -1) {
        ESP_LOGI(TAG, "Internet access confirmed: %s", line.c_str());
        return true;
    }

    ESP_LOGW(TAG, "Invalid HTTP response: %s", line.c_str());
    return false;
}

bool MyWiFiManager::validateWifiAndInternet() {
    ESP_LOGI(TAG, "=== Starting WiFi and Internet Validation ===");

    // Step 1: Try to connect to WiFi
    WiFi.begin(); // No parameters = use stored credentials with full scan

    int attempts = 0;
    const int maxAttempts = 20; // 10 seconds timeout

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "WiFi connection failed after %d attempts", attempts);
        return false;
    }

    ESP_LOGI(TAG, "WiFi connected successfully");
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
    ESP_LOGI(TAG, "Signal strength: %d dBm", WiFi.RSSI());

    // Step 2: Verify internet access
    if (!hasInternetAccess()) {
        ESP_LOGE(TAG, "Internet access verification failed");
        WiFi.disconnect();
        return false;
    }

    ESP_LOGI(TAG, "=== WiFi and Internet Validation: SUCCESS ===");
    return true;
}
