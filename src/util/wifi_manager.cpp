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
    
    // Get configuration from RTC
    RTCConfigData& config = ConfigManager::getConfig();
    
    // Try to reconnect using saved credentials
    if (strlen(config.ssid) > 0) {
        ESP_LOGI(TAG, "Attempting to connect to saved SSID: %s", config.ssid);
        WiFi.begin(config.ssid);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            ESP_LOGD(TAG, "Connecting to WiFi... attempt %d", attempts + 1);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            ESP_LOGI(TAG, "WiFi reconnected successfully!");
            ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
            ConfigManager::setNetwork(config.ssid, WiFi.localIP().toString());
            
            // Save updated configuration to NVS
            ConfigManager& configMgr = ConfigManager::getInstance();
            configMgr.saveToNVS();
        } else {
            ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
        }
    } else {
        ESP_LOGW(TAG, "No saved WiFi credentials available for reconnection");
    }
}

void MyWiFiManager::setupStationMode() {
    // Station mode only - connect to saved WiFi without AP mode
    ESP_LOGI(TAG, "Connecting to WiFi in station mode...");
    
    RTCConfigData& config = ConfigManager::getConfig();
    
    if (strlen(config.ssid) > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.ssid);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 10 seconds timeout
            delay(500);
            ESP_LOGD(TAG, "Connecting to WiFi... attempt %d", attempts + 1);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            ESP_LOGI(TAG, "WiFi connected successfully!");
            ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
            ConfigManager::setNetwork(config.ssid, WiFi.localIP().toString());
            
            // Save updated IP address to NVS
            ConfigManager& configMgr = ConfigManager::getInstance();
            configMgr.saveToNVS();
            
            // Start mDNS in station mode
            if (MDNS.begin("mystation")) {
                ESP_LOGI(TAG, "mDNS responder started: http://mystation.local");
            }
        } else {
            ESP_LOGW(TAG, "Failed to connect to WiFi in station mode");
        }
    } else {
        ESP_LOGW(TAG, "No saved WiFi credentials available");
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
