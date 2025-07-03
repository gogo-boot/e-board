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

ConfigManager& configMgr = ConfigManager::getInstance();
extern ConfigOption g_configOption;

// The parameter hasValidConfig will be set to true if the configuration is valid
// the parameter is used to fast path the configuration mode without reloading the configuration
bool DeviceModeManager::hasValidConfiguration(bool &hasValidConfig) {
    
    // Load configuration from NVS
    bool configExists = configMgr.loadFromNVS();
    
    if (!configExists) {
        ESP_LOGI(TAG, "No configuration found in NVS");
        return false;
    }
    
    // Validate critical configuration fields
    RTCConfigData& config = ConfigManager::getConfig();
    hasValidConfig = (strlen(config.selectedStopId) > 0 && 
                          strlen(config.ssid) > 0 &&
                          strlen(config.selectedStopId) > 0 && // Ensure selectedStopId is not empty
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
    
    // Set configuration mode flag
    ConfigManager::setConfigMode(true);
    
    ConfigManager::setDefaults();

    // Setup WiFi with access point for configuration
    WiFiManager wm;
    MyWiFiManager::setupAPMode(wm);
    
    // Setup time synchronization
    TimeManager::setupNTPTime();
    
    // Get location if not already saved
    if (g_configOption.latitude == 0.0 && g_configOption.longitude == 0.0) {
        getLocationFromGoogle(g_configOption.latitude, g_configOption.longitude);
        // Get city name from lat/lon
        ESP_LOGI(TAG, "Fetching city name from lat/lon: (%f, %f)", g_configOption.latitude, g_configOption.longitude);

        g_configOption.cityName = getCityFromLatLon(g_configOption.latitude, g_configOption.longitude);
        
        // put cityName into RTCConfigData
        if (g_configOption.cityName.isEmpty()) {
            ESP_LOGE(TAG, "Failed to get city name from lat/lon");
            // Set a default city name if fetching fails
            g_configOption.cityName = "Unknown City";
        }
        ESP_LOGI(TAG, "City name set: %s", g_configOption.cityName);
    } else {
        ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", g_configOption.cityName, g_configOption.latitude, g_configOption.longitude);
    }

    // Get nearby stops for configuration interface
    getNearbyStops(g_configOption.latitude, g_configOption.longitude);
    
    // Start web server for configuration
    setupWebServer(server);
    
    ESP_LOGI(TAG, "Configuration mode active - web server running");
    ESP_LOGI(TAG, "Access configuration at: %s or http://mystation.local", g_configOption.ipAddress);
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
    g_configOption.latitude = config.latitude;
    g_configOption.longitude = config.longitude;
    ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", 
        config.cityName, g_configOption.latitude, g_configOption.longitude);
    
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
        if (getWeatherFromDWD(g_configOption.latitude, g_configOption.longitude, weather)) {
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
