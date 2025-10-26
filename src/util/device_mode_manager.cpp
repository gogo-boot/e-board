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
#include "util/timing_manager.h"
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
#if PRODUCTIONP==0
    ConfigManager::printConfiguration(false);
#endif

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

    if (!setupConnectivityAndTime()) {
        return; // Let main.cpp handle sleep
    }

    // === FLOWCHART IMPLEMENTATION ===
    // Step 1: Check if Departure is in Active Time
    bool isTransportActiveTime = TimingManager::isTransportActiveTime();
    ESP_LOGI(TAG, "Transport active time: %s", isTransportActiveTime ? "YES" : "NO");

    if (!isTransportActiveTime) {
        // Path: Outside active time -> Check if time to update weather
        bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();
        ESP_LOGI(TAG, "Outside transport hours - weather update needed: %s", needsWeatherUpdate ? "YES" : "NO");

        if (needsWeatherUpdate) {
            // Fetch and display full weather screen
            WeatherInfo weather;
            if (fetchWeatherData(weather)) {
                initializeDisplay(DisplayMode::WEATHER_ONLY, DisplayOrientation::LANDSCAPE);
                DisplayManager::displayWeatherFull(weather);
                TimingManager::markWeatherUpdated();
                ESP_LOGI(TAG, "Updated weather full screen (outside transport hours)");
            }
        } else {
            ESP_LOGI(TAG, "No weather update needed - going to sleep");
        }
        return;
    }

    // === IN TRANSPORT ACTIVE TIME ===
    // Step 2: Check what needs updating
    bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();
    bool needsTransportUpdate = TimingManager::isTimeForTransportUpdate();

    ESP_LOGI(TAG, "Update requirements - Weather: %s, Transport: %s",
             needsWeatherUpdate ? "YES" : "NO", needsTransportUpdate ? "YES" : "NO");

    // Initialize display for half-and-half mode
    initializeDisplay(DisplayMode::HALF_AND_HALF, DisplayOrientation::LANDSCAPE);

    DepartureData depart;
    WeatherInfo weather;
    bool hasTransport = false;
    bool hasWeather = false;

    if (needsWeatherUpdate && needsTransportUpdate) {
        // Path: Update both weather and departure
        ESP_LOGI(TAG, "Updating both weather and departure data");

        if (fetchWeatherData(weather)) {
            hasWeather = true;
            TimingManager::markWeatherUpdated();
        }

        if (fetchTransportData(depart)) {
            hasTransport = true;
            TimingManager::markTransportUpdated();
        }

        // Display both halves
        DisplayManager::displayHalfAndHalf(hasWeather ? &weather : nullptr,
                                           hasTransport ? &depart : nullptr);
        ESP_LOGI(TAG, "Updated both halves");
    } else if (needsTransportUpdate) {
        // Path: Update departure half only
        ESP_LOGI(TAG, "Updating departure data only");

        if (fetchTransportData(depart)) {
            hasTransport = true;
            TimingManager::markTransportUpdated();

            // Partial update - departure half only
            DisplayManager::displayHalfAndHalf(nullptr, &depart);
            ESP_LOGI(TAG, "Updated departure half only");
        }
    } else if (needsWeatherUpdate) {
        // Path: Update weather half only
        ESP_LOGI(TAG, "Updating weather data only");

        if (fetchWeatherData(weather)) {
            hasWeather = true;
            TimingManager::markWeatherUpdated();

            // Partial update - weather half only
            DisplayManager::displayHalfAndHalf(&weather, nullptr);
            ESP_LOGI(TAG, "Updated weather half only");
        }
    } else {
        // Path: No updates needed
        ESP_LOGI(TAG, "No updates required - going to sleep");
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

    // For weather-only mode, only check weather updates
    bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();

    // Mode-specific data fetching and display
    WeatherInfo weather;
    bool hasWeather = false;

    // Fetch weather data only if needed
    if (needsWeatherUpdate) {
        ESP_LOGI(TAG, "Fetching weather for location: (%f, %f)",
                 g_webConfigPageData.latitude, g_webConfigPageData.longitude);
        if (getGeneralWeatherFull(g_webConfigPageData.latitude,
                                  g_webConfigPageData.longitude, weather)) {
            printWeatherInfo(weather);
            hasWeather = true;
            TimingManager::markWeatherUpdated();
        } else {
            ESP_LOGE(TAG, "Failed to get weather information from DWD.");
        }
    } else {
        ESP_LOGI(TAG, "Skipping weather update - not due yet");
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

    // For departure-only mode, only check transport updates and active hours
    bool needsTransportUpdate = TimingManager::isTimeForTransportUpdate();
    bool isActiveTime = TimingManager::isTransportActiveTime();

    // Mode-specific data fetching and display
    RTCConfigData& config = ConfigManager::getConfig();
    DepartureData depart;
    bool hasTransport = false;

    // Fetch departure data only if needed and in active hours
    if (needsTransportUpdate && isActiveTime) {
        String stopIdToUse = strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

        if (stopIdToUse.length() > 0) {
            ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
                     stopIdToUse.c_str(), config.selectedStopName);

            if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
                printDepartInfo(depart);
                if (depart.departureCount > 0) hasTransport = true;
                TimingManager::markTransportUpdated();
            } else {
                ESP_LOGE(TAG, "Failed to get departure information from RMV.");
            }
        } else {
            ESP_LOGW(TAG, "No stop configured.");
        }
    } else if (!isActiveTime) {
        ESP_LOGI(TAG, "Outside transport active hours - skipping departure fetch");
    } else {
        ESP_LOGI(TAG, "Skipping transport update - not due yet");
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

    // Calculate sleep time using TimingManager based on configured intervals
    uint64_t sleepTime = TimingManager::getNextSleepDuration();
    ESP_LOGI(TAG, "Entering deep sleep for %llu microseconds (%.1f minutes)", sleepTime,
             sleepTime / (60.0 * 1000000.0));
    enterDeepSleep(sleepTime);
}

// ===== HELPER FUNCTIONS FOR DATA FETCHING =====

bool DeviceModeManager::fetchWeatherData(WeatherInfo& weather) {
    ESP_LOGI(TAG, "Fetching weather for location: (%f, %f)",
             g_webConfigPageData.latitude, g_webConfigPageData.longitude);
 if (getGeneralWeatherFull(g_webConfigPageData.latitude,
                              g_webConfigPageData.longitude, weather)) {
        printWeatherInfo(weather);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to get weather information");
        return false;
    }
}

bool DeviceModeManager::fetchTransportData(DepartureData& depart) {
    RTCConfigData& config = ConfigManager::getConfig();
    String stopIdToUse = strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

    if (stopIdToUse.length() == 0) {
        ESP_LOGW(TAG, "No stop configured for transport data");
        return false;
    }

    ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
             stopIdToUse.c_str(), config.selectedStopName);

    if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
        printDepartInfo(depart);
        if (depart.departureCount > 0) {
            return true;
        } else {
            ESP_LOGW(TAG, "No departures found for stop");
            return false;
        }
    } else {
        ESP_LOGE(TAG, "Failed to get departure information from RMV");
        return false;
    }
}
