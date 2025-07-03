#include "wifi_manager.h"
#include "config/config_manager.h"
#include "util/util.h"
#include "esp_log.h"

static const char* TAG = "WIFI_MGR";

void MyWiFiManager::reconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGD(TAG, "WiFi already connected: %s", WiFi.localIP().toString().c_str());
        return; // Already connected
    }
    
    ESP_LOGI(TAG, "WiFi disconnected, attempting to reconnect...");
    
    // Try to reconnect using ESP32's stored credentials (from WiFiManager)
    // This will use the credentials saved by WiFiManager automatically
    WiFi.begin();  // No parameters = use stored credentials
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        ESP_LOGD(TAG, "Connecting to WiFi... attempt %d", attempts + 1);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi reconnected successfully!");
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "Connected to SSID: %s", WiFi.SSID().c_str());

    } else {
        ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
    }
}

void MyWiFiManager::setupAPMode(WiFiManager &wm) {
    ESP_LOGD(TAG, "Starting WiFiManager AP mode...");
    const char *menu[] = {"wifi"};
    wm.setMenu(menu, 1);
    wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    String apName = Util::getUniqueSSID("MyStation");
    ESP_LOGD(TAG, "AP SSID: %s", apName.c_str());
    bool res = wm.autoConnect(apName.c_str());
    ESP_LOGD(TAG, "autoConnect() returned");
    if (!res) {
        ESP_LOGE(TAG, "Failed to connect");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected!");
    
    // Update configuration with new network info
    ConfigManager::setNetwork(wm.getWiFiSSID(), WiFi.localIP().toString());
    
    // Save WiFi credentials immediately to NVS
    ConfigManager& configMgr = ConfigManager::getInstance();
    ESP_LOGI(TAG, "Saving WiFi credentials to NVS: SSID=%s, IP=%s", 
             wm.getWiFiSSID().c_str(), WiFi.localIP().toString().c_str());
    configMgr.saveToNVS();
    
    if (MDNS.begin("mystation")) {
        ESP_LOGI(TAG, "mDNS responder started: http://mystation.local");
    } else {
        ESP_LOGW(TAG, "mDNS responder failed to start");
    }
}

bool MyWiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String MyWiFiManager::getLocalIP() {
    return WiFi.localIP().toString();
}
