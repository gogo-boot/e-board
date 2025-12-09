#include "util/wifi_manager.h"
#include "config/config_manager.h"
#include "util/util.h"

static const char* TAG = "WIFI_MGR";

// RTC variables to persist WiFi state across deep sleep


void MyWiFiManager::reconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGD(TAG, "WiFi already connected: %s", WiFi.localIP().toString().c_str());
        return; // Already connected
    }

    WiFi.begin();

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
    } else {
        ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
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

    // Start AP
    // It holds further process until user configures WiFi
    wm.autoConnect(apName.c_str());

    // Check internet connectivity
    if (hasInternetAccess()) {
        ESP_LOGW(TAG, "has internet access");
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
