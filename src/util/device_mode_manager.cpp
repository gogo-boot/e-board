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
#include "config/config_page_data.h"
#include "config/config_struct.h"
#include "display/display_manager.h"
#include "util/transport_print.h"
#include "util/sleep_utils.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "util/util.h"
#include "util/weather_print.h"
#include "util/wifi_manager.h"

static const char* TAG = "DEVICE_MODE";

// Global variables needed for operation
extern WebServer server;

ConfigManager& configMgr = ConfigManager::getInstance();
RTCConfigData& config = ConfigManager::getConfig();

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
    bool hasStopId = strlen(config.selectedStopId) > 0;
    bool hasSSID = strlen(config.ssid) > 0;
    bool hasLocation = (config.latitude != 0.0 && config.longitude != 0.0);
    hasValidConfig = hasStopId && hasSSID && hasLocation;

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

    // Phase 2+: WiFi already configured, setup app configuration
    ESP_LOGI(TAG, "=== PHASE 2+ CONFIGURATION MODE ===");
    ESP_LOGI(TAG, "WiFi already configured, setting up app configuration...");

    // Ensure WiFi connection
    if (!MyWiFiManager::isConnected()) {
        ESP_LOGW(TAG, "WiFi not connected, attempting reconnect...");
        MyWiFiManager::reconnectWiFi();
    }

    // Setup time synchronization
    TimeManager::setupNTPTime();

    // Get location if not already saved
    ConfigPageData& pageData = ConfigPageData::getInstance();

    pageData.setIPAddress(config.ipAddress);

    // if (pageData.getLatitude() == 0.0 && pageData.getLongitude() == 0.0) {
    float lat, lon;
    getLocationFromGoogle(lat, lon);

    ESP_LOGI(TAG, "Fetching city name from lat/lon: (%f, %f)", lat, lon);
    String cityName = getCityFromLatLon(lat, lon);

    if (cityName.isEmpty()) {
        ESP_LOGE(TAG, "Failed to get city name from lat/lon");
        cityName = "Unknown City";
    }
    pageData.setLocation(lat, lon, cityName);
    ESP_LOGI(TAG, "City name set: %s", cityName.c_str());

    // Get nearby stops for configuration interface
    getNearbyStops(pageData.getLatitude(), pageData.getLongitude());

    // Start web server for configuration
    setupWebServer(server);

    // Start mDNS responder
    if (MDNS.begin("mystation")) {
        ESP_LOGI(TAG, "mDNS started: http://mystation.local");
    } else {
        ESP_LOGW(TAG, "mDNS failed to start");
    }
    ESP_LOGI(TAG, "Configuration web server started");
    ESP_LOGI(TAG, "Access configuration at: %s or http://mystation.local",
             config.ipAddress);
    ESP_LOGI(TAG, "Web server will handle configuration until user saves settings");
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

    if (!config.inTemporaryMode && !isTransportActiveTime) {
        // Path: Outside active time -> Check if time to update weather
        bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();
        ESP_LOGI(TAG, "Outside transport hours - weather update needed: %s", needsWeatherUpdate ? "YES" : "NO");

        if (!config.inTemporaryMode && needsWeatherUpdate) {
            // Fetch and display full weather screen
            WeatherInfo weather;
            if (fetchWeatherData(weather)) {
                DisplayManager::refreshWeatherFullScreen(weather);
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

    DepartureData depart;
    WeatherInfo weather;
    bool hasTransport = false;
    bool hasWeather = false;

    if (config.inTemporaryMode || (needsWeatherUpdate && needsTransportUpdate)) {
        // Path: Update both weather and departure - FULL REFRESH
        ESP_LOGI(TAG, "Updating both weather and departure data");

        if (fetchWeatherData(weather)) {
            hasWeather = true;
            TimingManager::markWeatherUpdated();
        }

        if (fetchTransportData(depart)) {
            hasTransport = true;
            TimingManager::markTransportUpdated();
        }

        // Display both halves using centralized method
        DisplayManager::refreshFullScreen(hasWeather ? &weather : nullptr,
                                          hasTransport ? &depart : nullptr);
        ESP_LOGI(TAG, "Updated both halves");
    } else if (config.inTemporaryMode || needsTransportUpdate) {
        // Path: Update departure half only - PARTIAL UPDATE
        ESP_LOGI(TAG, "Updating departure data only");

        if (fetchTransportData(depart)) {
            hasTransport = true;
            TimingManager::markTransportUpdated();

            // Partial update using centralized method
            DisplayManager::refreshDepartureHalf(&depart);
            ESP_LOGI(TAG, "Updated departure half only");
        }
    } else if (config.inTemporaryMode || needsWeatherUpdate) {
        // Path: Update weather half only - PARTIAL UPDATE
        ESP_LOGI(TAG, "Updating weather data only");

        if (fetchWeatherData(weather)) {
            hasWeather = true;
            TimingManager::markWeatherUpdated();

            // Partial update using centralized method
            DisplayManager::refreshWeatherHalf(&weather);
            ESP_LOGI(TAG, "Updated weather half only");
        }
    } else {
        // Path: No updates needed
        ESP_LOGI(TAG, "No updates required - going to sleep");
    }
}

void DeviceModeManager::updateWeatherFull() {
    // Use common operational setup
    if (!setupOperationalMode()) {
        return; // setupOperationalMode already handles error case
    }

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
    if (config.inTemporaryMode || needsWeatherUpdate) {
        // Use RTC config which persists across deep sleep
        ESP_LOGI(TAG, "Fetching weather for location: %s (%.6f, %.6f)",
                 config.cityName, config.latitude, config.longitude);
        if (getGeneralWeatherFull(config.latitude, config.longitude, weather)) {
            printWeatherInfo(weather);
            hasWeather = true;
            TimingManager::markWeatherUpdated();
        } else {
            ESP_LOGE(TAG, "Failed to get weather information from DWD.");
        }
    } else {
        ESP_LOGI(TAG, "Skipping weather update - not due yet");
    }

    // Display using centralized refresh method
    if (hasWeather) {
        ESP_LOGI(TAG, "Displaying weather data");
        DisplayManager::refreshWeatherFullScreen(weather);
    } else {
        ESP_LOGW(TAG, "No data to display");
    }
}

void DeviceModeManager::updateDepartureFull() {
    // Use common operational setup
    if (!setupOperationalMode()) {
        return; // setupOperationalMode already handles error case
    }

    // Setup connectivity and time
    if (!setupConnectivityAndTime()) {
        return; // Let main.cpp handle sleep
    }

    // For departure-only mode, only check transport updates and active hours
    bool needsTransportUpdate = TimingManager::isTimeForTransportUpdate();
    bool isActiveTime = TimingManager::isTransportActiveTime();

    // Mode-specific data fetching and display
    DepartureData depart;
    bool hasTransport = false;

    // Fetch departure data only if needed and in active hours
    if (config.inTemporaryMode || (needsTransportUpdate && isActiveTime)) {
        String stopIdToUse = strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

        if (stopIdToUse.length() > 0) {
            ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
                     stopIdToUse.c_str(), config.selectedStopName);

            if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
                printTransportInfo(depart);
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

    // Display using centralized refresh method
    if (hasTransport) {
        ESP_LOGI(TAG, "Displaying departure data");
        DisplayManager::refreshDepartureFullScreen(depart);
    } else {
        ESP_LOGW(TAG, "No data to display");
    }
}

// ===== COMMON OPERATIONAL MODE FUNCTIONS =====

bool DeviceModeManager::setupOperationalMode() {
    ESP_LOGI(TAG, "=== ENTERING OPERATIONAL MODE ===");

    // Set operational mode flag
    ConfigManager::setConfigMode(false);

    // Validate RTC config (should already be loaded by system_init)
    // DO NOT reload from NVS here - it would overwrite RTC memory including:
    // - WiFi cache state
    // - Temporary button mode flags
    // - Loop counters
    if (!ConfigManager::hasValidConfig()) {
        ESP_LOGE(TAG, "No valid configuration available!");
        ESP_LOGI(TAG, "Switching to configuration mode...");
        runConfigurationMode();
        return false;
    }

    // Check if this is a deep sleep wake-up for fast path
    if (!ConfigManager::isFirstBoot()) {
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
            ESP_LOGI(TAG, "Time since last sync: %lu ms (%s)",
                     timeSinceSync, TimeManager::formatDurationInHours(timeSinceSync).c_str());

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
            ESP_LOGI(TAG, "Time since last sync: %lu ms (%s)",
                     timeSinceSync, TimeManager::formatDurationInHours(timeSinceSync).c_str());
        }
        // Always print current time for verification
        TimeManager::printCurrentTime();
        return true;
    } else {
        ESP_LOGW(TAG, "WiFi not connected - cannot fetch data");
        return false;
    }
}


void DeviceModeManager::enterOperationalSleep() {
    // Clear temporary mode flag (button press is one-time only)
    RTCConfigData& config = ConfigManager::getConfig();
    if (config.inTemporaryMode) {
        ESP_LOGI(TAG, "Clearing temporary mode flag before sleep");
        config.inTemporaryMode = false;
        config.temporaryDisplayMode = 0xFF;
        config.temporaryModeStartTime = 0;
    }
    // Save WiFi state to RTC memory before hibernating for fast reconnect after deep sleep
    if (MyWiFiManager::isConnected()) {
        MyWiFiManager::saveWiFiStateToRTC();
    }

    // Hibernate display to save power
    DisplayManager::hibernate();

    // Calculate sleep time using TimingManager based on configured intervals
    uint64_t sleepTimeSeconds = TimingManager::getNextSleepDurationSeconds();

    // On other boards, use regular timer-only deep sleep
    enterDeepSleep(sleepTimeSeconds);
}

// ===== HELPER FUNCTIONS FOR DATA FETCHING =====

bool DeviceModeManager::fetchWeatherData(WeatherInfo& weather) {
    // Use RTC config which persists across deep sleep
    ESP_LOGI(TAG, "Fetching weather for location: %s (%.6f, %.6f)",
             config.cityName, config.latitude, config.longitude);
    if (getGeneralWeatherFull(config.latitude, config.longitude, weather)) {
        printWeatherInfo(weather);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to get weather information");
        return false;
    }
}

bool DeviceModeManager::fetchTransportData(DepartureData& depart) {
    String stopIdToUse = strlen(config.selectedStopId) > 0 ? String(config.selectedStopId) : "";

    if (stopIdToUse.length() == 0) {
        ESP_LOGW(TAG, "No stop configured for transport data");
        return false;
    }

    ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
             stopIdToUse.c_str(), config.selectedStopName);

    if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
        printTransportInfo(depart);
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

// ===== CONFIGURATION PHASE MANAGEMENT =====

ConfigPhase DeviceModeManager::getCurrentPhase() {
    // Phase 1: WiFi not configured or credentials empty
    if (!config.wifiConfigured || strlen(config.ssid) == 0) {
        ESP_LOGI(TAG, "Configuration Phase: 1 (WiFi Setup)");
        return PHASE_WIFI_SETUP;
    }

    // Phase 2: WiFi configured but app settings missing
    if (strlen(config.selectedStopId) == 0 ||
        config.latitude == 0.0 ||
        config.longitude == 0.0) {
        ESP_LOGI(TAG, "Configuration Phase: 2 (Application Setup)");
        return PHASE_APP_SETUP;
    }

    // Phase 3: Everything configured
    ESP_LOGI(TAG, "Configuration Phase: 3 (Complete)");
    return PHASE_COMPLETE;
}

void DeviceModeManager::showPhaseInstructions(ConfigPhase phase) {
    // Display instructions on e-paper and log them

    switch (phase) {
    case PHASE_WIFI_SETUP: {
        ESP_LOGI(TAG, "=== SETUP - Schritt 1/2: WiFi-Konfiguration ===");

        // Display Phase 1 instructions on e-paper (in German)
        DisplayManager::displayPhase1WifiSetup();
    }
    break;

    case PHASE_APP_SETUP:
        ESP_LOGI(TAG, "=== SETUP - Schritt 2/2: Stations-Konfiguration ===");

        // Display Phase 2 instructions on e-paper (in German)
        DisplayManager::displayPhase2AppSetup();

        break;

    case PHASE_COMPLETE:
        ESP_LOGI(TAG, "=== Configuration Complete ===");
        ESP_LOGI(TAG, "System will enter operational mode");
        break;
    }
}

void DeviceModeManager::showWifiErrorPage() {
    ESP_LOGE(TAG, "=== INTERNET ACCESS ERROR ===");
    ESP_LOGE(TAG, "WiFi connected but internet is not accessible");
    ESP_LOGE(TAG, "");

    // Also log to serial
    ESP_LOGI(TAG, "WiFi: Connected ✓");
    ESP_LOGI(TAG, "1. Open browser: http://192.168.4.1 or http://mystation.local");
    ESP_LOGI(TAG, "2. Select your transport station");
    ESP_LOGI(TAG, "3. Configure display settings and intervals");
    ESP_LOGI(TAG, "4. Save configuration to begin operation");
}
