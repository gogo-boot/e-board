#include <Arduino.h>
#include "device_mode_manager.h"
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "config/config_page.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "sleep_utils.h"
#include "weather_print.h"
#include "api/rmv_api.h"
#include "api/google_api.h"
#include "api/dwd_weather_api.h"
#include "esp_log.h"
#include <WiFiManager.h>
#include <LittleFS.h>
#include <WebServer.h>

static const char* TAG = "DEVICE_MODE";

// Global variables needed for operation
extern float g_lat, g_lon;
extern WebServer server;

bool DeviceModeManager::hasValidConfiguration() {
    ConfigManager& configMgr = ConfigManager::getInstance();
    
    // Load configuration from NVS
    bool configExists = configMgr.loadFromNVS();
    
    if (!configExists) {
        ESP_LOGI(TAG, "No configuration found in NVS");
        return false;
    }
    
    // Validate critical configuration fields
    RTCConfigData& config = ConfigManager::getConfig();
    bool hasValidConfig = (strlen(config.selectedStopId) > 0 && 
                          strlen(config.ssid) > 0 &&
                          config.latitude != 0.0 && 
                          config.longitude != 0.0);
    
    ESP_LOGI(TAG, "- SSID: %s", config.ssid);
    ESP_LOGI(TAG, "- Stop: %s (%s)", config.selectedStopName, config.selectedStopId);
    ESP_LOGI(TAG, "- Location: %s (%f, %f)", config.cityName, config.latitude, config.longitude);
    ESP_LOGI(TAG, "- IP Address: %s", config.ipAddress);

    if (hasValidConfig) {
        ESP_LOGI(TAG, "Valid configuration found in NVS");
        return true;
    } else {
        ESP_LOGI(TAG, "Incomplete configuration in NVS, missing critical fields");
        return false;
    }
}

void DeviceModeManager::runConfigurationMode() {
    ESP_LOGI(TAG, "=== ENTERING CONFIGURATION MODE ===");
    
    ConfigManager& configMgr = ConfigManager::getInstance();
    
    // Set configuration mode flag
    ConfigManager::setConfigMode(true);
    
    // Initialize config with defaults if needed
    if (!ConfigManager::hasValidConfig()) {
        ConfigManager::setDefaults();
    }

    // Initialize filesystem
    if (!LittleFS.begin()) {
        ESP_LOGE(TAG, "LittleFS mount failed! Please check filesystem or flash.");
        while (true) {
            delay(1000);
        }
    }

    // Setup WiFi with access point for configuration
    WiFiManager wm;
    MyWiFiManager::setupAPMode(wm);
    
    // Setup time synchronization
    TimeManager::setupNTPTime();
    
    // Load any existing partial configuration
    configMgr.loadFromNVS();
    
    // Get location if not already saved
    RTCConfigData& config = ConfigManager::getConfig();
    if (config.latitude == 0.0 && config.longitude == 0.0) {
        getLocationFromGoogle(g_lat, g_lon);
        getCityFromLatLon(g_lat, g_lon);
        ESP_LOGI(TAG, "City set in setup: %s", config.cityName);
    } else {
        // Use saved coordinates
        g_lat = config.latitude;
        g_lon = config.longitude;
        ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
        config.cityName, g_lat, g_lon);
    }

    // Get nearby stops for configuration interface
    getNearbyStops();
    
    // Start web server for configuration
    setupWebServer(server);
    
    ESP_LOGI(TAG, "Configuration mode active - web server running");
    ESP_LOGI(TAG, "Access configuration at: %s or http://mystation.local", config.ipAddress);
}

void DeviceModeManager::runOperationalMode() {
    ESP_LOGI(TAG, "=== ENTERING OPERATIONAL MODE ===");
    
    ConfigManager& configMgr = ConfigManager::getInstance();
    
    // Set operational mode flag
    ConfigManager::setConfigMode(false);
    
    // Load complete configuration from NVS
    if (!configMgr.loadFromNVS()) {
        ESP_LOGE(TAG, "Failed to load configuration in operational mode!");
        ESP_LOGI(TAG, "Switching to configuration mode...");
        runConfigurationMode();
        return;
    }
    
    // Set coordinates from saved config
    RTCConfigData& config = ConfigManager::getConfig();
    g_lat = config.latitude;
    g_lon = config.longitude;
    ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
        config.cityName, g_lat, g_lon);
    
    // Check if this is a deep sleep wake-up for fast path
    if (!ConfigManager::isFirstBoot() && ConfigManager::hasValidConfig()) {
        ESP_LOGI(TAG, "Fast wake: Using RTC config after deep sleep");
        extern unsigned long loopCount;
        loopCount++;
        
        // Print wakeup reason and current time
        printWakeupReason();
        ESP_LOGI(TAG, "Loop count: %lu", loopCount);
        
        TimeManager::printCurrentTime();
    }
    
    // Connect to WiFi in station mode
    MyWiFiManager::setupStationMode();
    
    if (MyWiFiManager::isConnected()) {
        // Fetch and display data
        String stopIdToUse = strlen(config.selectedStopId) > 0 ? 
                             String(config.selectedStopId) : "";
        
        if (stopIdToUse.length() > 0) {
            ESP_LOGI(TAG, "Using stop ID: %s (%s)", stopIdToUse.c_str(), config.selectedStopName);
            getDepartureBoard(stopIdToUse.c_str());
        } else {
            ESP_LOGW(TAG, "No stop configured.");
        }
        
        WeatherInfo weather;
        if (getWeatherFromDWD(g_lat, g_lon, weather)) {
            printWeatherInfo(weather);
        } else {
            ESP_LOGE(TAG, "Failed to get weather information from DWD.");
        }
    } else {
        ESP_LOGW(TAG, "WiFi not connected");
    }

    // Calculate sleep time and enter deep sleep
    uint64_t sleepTime = calculateSleepTime(config.transportInterval);
    ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds", sleepTime);
    enterDeepSleep(sleepTime);
}
