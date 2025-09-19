#include "util/device_mode_manager.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <esp_log.h>

#include "api/dwd_weather_api.h"
#include "api/google_api.h"
#include "api/rmv_api.h"
#include "config/config_manager.h"
#include "config/config_page.h"
#include "config/config_struct.h"
#include "display/display_manager.h"
#include "util/departure_print.h"
#include "util/sleep_utils.h"
#include "util/time_manager.h"
#include "util/weather_print.h"
#include "util/wifi_manager.h"

static const char* TAG = "DEVICE_MODE";

// Global variables needed for operation
extern float g_lat, g_lon;
extern WebServer server;

ConfigManager& configMgr = ConfigManager::getInstance();
extern ConfigOption g_webConfigPageData;

// The parameter hasValidConfig will be set to true if the configuration is
// valid the parameter is used to fast path the configuration mode without
// reloading the configuration
bool DeviceModeManager::hasValidConfiguration(bool& hasValidConfig) {
    // Load configuration from NVS
    bool configExists = configMgr.loadFromNVS();

    if (!configExists) {
        ESP_LOGI(TAG, "No configuration found in NVS");
        return false;
    }

    // Validate critical configuration fields
    RTCConfigData& config = ConfigManager::getConfig();
    hasValidConfig =
    (strlen(config.selectedStopId) > 0 && strlen(config.ssid) > 0 &&
        strlen(config.selectedStopId) >
        0 && // Ensure selectedStopId is not empty
        config.latitude != 0.0 &&
        config.longitude != 0.0);

    ESP_LOGI(TAG, "- SSID: %s", config.ssid);
    ESP_LOGI(TAG, "- Stop: %s (%s)", config.selectedStopName,
             config.selectedStopId);
    ESP_LOGI(TAG, "- Location: %s (%f, %f)", config.cityName, config.latitude,
             config.longitude);
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
    if (g_webConfigPageData.latitude == 0.0 &&
        g_webConfigPageData.longitude == 0.0) {
        getLocationFromGoogle(g_webConfigPageData.latitude,
                              g_webConfigPageData.longitude);
        // Get city name from lat/lon
        ESP_LOGI(TAG, "Fetching city name from lat/lon: (%f, %f)",
                 g_webConfigPageData.latitude, g_webConfigPageData.longitude);

        g_webConfigPageData.cityName = getCityFromLatLon(
            g_webConfigPageData.latitude, g_webConfigPageData.longitude);

        // put cityName into RTCConfigData
        if (g_webConfigPageData.cityName.isEmpty()) {
            ESP_LOGE(TAG, "Failed to get city name from lat/lon");
            // Set a default city name if fetching fails
            g_webConfigPageData.cityName = "Unknown City";
        }
        ESP_LOGI(TAG, "City name set: %s", g_webConfigPageData.cityName);
    } else {
        ESP_LOGI(TAG, "Using saved location: %s (%f, %f)",
                 g_webConfigPageData.cityName, g_webConfigPageData.latitude,
                 g_webConfigPageData.longitude);
    }

    // Get nearby stops for configuration interface
    getNearbyStops(g_webConfigPageData.latitude, g_webConfigPageData.longitude);

    // Start web server for configuration
    setupWebServer(server);

    ESP_LOGI(TAG, "Configuration mode active - web server running");
    ESP_LOGI(TAG, "Access configuration at: %s or http://mystation.local",
             g_webConfigPageData.ipAddress);
}

void DeviceModeManager::showWeatherDeparture() {
    // Use common operational setup
    if (!setupOperationalMode()) {
        return; // setupOperationalMode already handles error case
    }

    // Initialize display for half-and-half mode
    initializeDisplay(DisplayMode::HALF_AND_HALF, DisplayOrientation::LANDSCAPE);

    // Setup connectivity and time
    if (!setupConnectivityAndTime()) {
        return; // Let main.cpp handle sleep
    }

    // Mode-specific data fetching and display
    RTCConfigData& config = ConfigManager::getConfig();
    DepartureData depart;
    WeatherInfo weather;
    bool hasTransport = false;
    bool hasWeather = false;

    // Fetch departure data
    String stopIdToUse =
        strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

    if (stopIdToUse.length() > 0) {
        ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
                 stopIdToUse.c_str(), config.selectedStopName);

        if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
            printDepartInfo(depart);
            if (depart.departureCount > 0) hasTransport = true;
        } else {
            ESP_LOGE(TAG, "Failed to get departure information from RMV.");
        }
    } else {
        ESP_LOGW(TAG, "No stop configured.");
    }

    // Fetch weather data
    ESP_LOGI(TAG, "Fetching weather for location: (%f, %f)",
             g_webConfigPageData.latitude, g_webConfigPageData.longitude);
    if (getGeneralWeatherHalf(g_webConfigPageData.latitude,
                              g_webConfigPageData.longitude, weather)) {
        printWeatherInfo(weather);
        hasWeather = true;
    } else {
        ESP_LOGE(TAG, "Failed to get weather information from DWD.");
    }

    // Display using new display manager
    if (hasWeather || hasTransport) {
        ESP_LOGI(TAG, "Displaying both weather and transport data");
        DisplayManager::displayHalfAndHalf(&weather, &depart);
    } else {
        ESP_LOGW(TAG, "No data to display");
    }
}

void DeviceModeManager::showGeneralWeather() {
    // Use common operational setup
    if (!setupOperationalMode()) {
        return; // setupOperationalMode already handles error case
    }

    // Initialize display for weather-only mode
    initializeDisplay(DisplayMode::WEATHER_ONLY, DisplayOrientation::LANDSCAPE);

    // Setup connectivity and time
    if (!setupConnectivityAndTime()) {
        return; // Let main.cpp handle sleep
    }

    // Mode-specific data fetching and display
    WeatherInfo weather;
    bool hasWeather = false;

    // Fetch weather data
    ESP_LOGI(TAG, "Fetching weather for location: (%f, %f)",
             g_webConfigPageData.latitude, g_webConfigPageData.longitude);
    if (getGeneralWeatherFull(g_webConfigPageData.latitude,
                              g_webConfigPageData.longitude, weather)) {
        printWeatherInfo(weather);
        hasWeather = true;
    } else {
        ESP_LOGE(TAG, "Failed to get weather information from DWD.");
    }

    // Display using new display manager
    if (hasWeather) {
        ESP_LOGI(TAG, "Displaying weather data");
        DisplayManager::displayWeatherFull(weather);
    } else {
        ESP_LOGW(TAG, "No data to display");
    }
}

void DeviceModeManager::showMarineWeather() {}

void DeviceModeManager::showDeparture() {
    // Use common operational setup
    if (!setupOperationalMode()) {
        return; // setupOperationalMode already handles error case
    }

    // Initialize display for departure-only mode (using HALF_AND_HALF for full departures)
    initializeDisplay(DisplayMode::HALF_AND_HALF, DisplayOrientation::LANDSCAPE);

    // Setup connectivity and time
    if (!setupConnectivityAndTime()) {
        return; // Let main.cpp handle sleep
    }

    // Mode-specific data fetching and display
    RTCConfigData& config = ConfigManager::getConfig();
    DepartureData depart;
    bool hasTransport = false;

    // Fetch departure data
    String stopIdToUse =
        strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

    if (stopIdToUse.length() > 0) {
        ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
                 stopIdToUse.c_str(), config.selectedStopName);

        if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
            printDepartInfo(depart);
            if (depart.departureCount > 0) hasTransport = true;
        } else {
            ESP_LOGE(TAG, "Failed to get departure information from RMV.");
        }
    } else {
        ESP_LOGW(TAG, "No stop configured.");
    }

    // Display using new display manager
    if (hasTransport) {
        ESP_LOGI(TAG, "Displaying departure data");
        DisplayManager::displayDeparturesFull(depart);
    } else {
        ESP_LOGW(TAG, "No data to display");
    }
}

// ===== COMMON OPERATIONAL MODE FUNCTIONS =====

bool DeviceModeManager::setupOperationalMode() {
    ESP_LOGI(TAG, "=== ENTERING OPERATIONAL MODE ===");

    ConfigManager& configMgr = ConfigManager::getInstance();

    // Set operational mode flag
    ConfigManager::setConfigMode(false);

    // Load complete configuration from NVS
    if (!configMgr.loadFromNVS()) {
        ESP_LOGE(TAG, "Failed to load configuration in operational mode!");
        ESP_LOGI(TAG, "Switching to configuration mode...");
        runConfigurationMode();
        return false;
    }

    // Set coordinates from saved config
    RTCConfigData& config = ConfigManager::getConfig();
    g_webConfigPageData.latitude = config.latitude;
    g_webConfigPageData.longitude = config.longitude;
    ESP_LOGI(TAG, "Using saved location: %s (%f, %f)", config.cityName,
             g_webConfigPageData.latitude, g_webConfigPageData.longitude);

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

    return true;
}

bool DeviceModeManager::setupConnectivityAndTime() {
    // Connect to WiFi in station mode
    MyWiFiManager::reconnectWiFi();

    if (MyWiFiManager::isConnected()) {
        // Enhanced time synchronization logic for deep sleep optimization
        bool timeIsSet = TimeManager::isTimeSet();
        bool needsSync = TimeManager::needsPeriodicSync();

        if (!timeIsSet) {
            // Time is not set at all - force NTP sync
            ESP_LOGI(TAG, "Time not set, performing initial NTP synchronization...");
            if (TimeManager::setupNTPTimeWithRetry(3)) {
                ESP_LOGI(TAG, "Initial NTP sync successful");
            } else {
                ESP_LOGE(TAG, "Failed to sync time via NTP");
                return false; // Cannot proceed without time
            }
        } else if (needsSync) {
            // Time is set but needs periodic refresh due to RTC drift
            ESP_LOGI(TAG, "Time needs periodic refresh - performing NTP sync...");
            unsigned long timeSinceSync = TimeManager::getTimeSinceLastSync();
            ESP_LOGI(TAG, "Time since last sync: %lu ms (%.1f hours)",
                     timeSinceSync, timeSinceSync / (1000.0 * 60.0 * 60.0));

            if (TimeManager::setupNTPTimeWithRetry(2)) {
                ESP_LOGI(TAG, "Periodic NTP sync successful");
            } else {
                ESP_LOGW(TAG, "Periodic NTP sync failed - continuing with RTC time");
                // Continue with RTC time - not critical for operation
            }
        } else {
            // Time is set and recent - use RTC time (most efficient path)
            ESP_LOGI(TAG, "Using RTC time - no sync needed");
            unsigned long timeSinceSync = TimeManager::getTimeSinceLastSync();
            ESP_LOGI(TAG, "Time since last sync: %lu ms (%.1f hours)",
                     timeSinceSync, timeSinceSync / (1000.0 * 60.0 * 60.0));
        }
        // Always print current time for verification
        TimeManager::printCurrentTime();
        return true;
    } else {
        ESP_LOGW(TAG, "WiFi not connected - cannot fetch data");
        return false;
    }
}

void DeviceModeManager::initializeDisplay(DisplayMode mode, DisplayOrientation orientation) {
    // Initialize display manager (can be configured via config later)
    DisplayManager::init(orientation);
    DisplayManager::setMode(mode, orientation);
}

void DeviceModeManager::enterOperationalSleep() {
    // Save WiFi state to RTC memory before hibernating for fast reconnect after deep sleep
    if (MyWiFiManager::isConnected()) {
        MyWiFiManager::saveWiFiStateToRTC();
    }
    // Hibernate display to save power
    DisplayManager::hibernate();

    // Calculate sleep time and enter deep sleep
    // uint64_t sleepTime = calculateSleepTime(config.transportInterval);
    uint64_t sleepTime = calculateSleepTime(1);
    ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds", sleepTime);
    enterDeepSleep(sleepTime);
}
