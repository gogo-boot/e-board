#include "util/device_mode_manager.h"

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

#include "api/dwd_weather_api.h"
#include "api/google_api.h"
#include "api/rmv_api.h"
#include "config/config_manager.h"
#include "config/config_page.h"
#include "config/config_page_data.h"
#include "config/config_struct.h"
#include "display/display_manager.h"
#include "util/transport_print.h"
#include "global_instances.h"

#include "util/sleep_utils.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "util/weather_print.h"
#include "util/wifi_manager.h"

static const char* TAG = "DEVICE_MODE";

// Global variables needed for operation

ConfigManager& configMgr = ConfigManager::getInstance();
RTCConfigData& config = ConfigManager::getConfig();
RTC_DATA_ATTR WeatherInfo weather;

void DeviceModeManager::runConfigurationMode() {
    ESP_LOGI(TAG, "=== ENTERING CONFIGURATION MODE ===");

    // Phase 2+: WiFi already configured, setup app configuration
    ESP_LOGI(TAG, "=== PHASE 2+ CONFIGURATION MODE ===");
    ESP_LOGI(TAG, "WiFi already configured, setting up app configuration...");

    // Ensure WiFi connection
    if (!MyWiFiManager::isConnected()) {
        ESP_LOGW(TAG, "WiFi not connected, attempting reconnect...");
        MyWiFiManager::reconnectWiFi();
    }

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
    // Path: Outside active time -> Check if time to update weather
    bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();
    ESP_LOGI(TAG, "Update requirements - Weather: %s", needsWeatherUpdate ? "YES" : "NO");

    DepartureData depart;

    // Path: Update both weather and departure - FULL REFRESH
    ESP_LOGI(TAG, "Updating both weather and departure data");

    if (needsWeatherUpdate && getGeneralWeatherFull(config.latitude, config.longitude, weather)) {
        printWeatherInfo(weather);
        TimingManager::markWeatherUpdated();
    }
    fetchTransportData(depart);
    TimingManager::markTransportUpdated();

    DisplayManager::displayHalfNHalf(weather, depart);
}

void DeviceModeManager::updateWeatherFull() {
    // For weather-only mode, only check weather updates
    bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();

    // Fetch weather data only if needed
    if (needsWeatherUpdate) {
        // Use RTC config which persists across deep sleep
        ESP_LOGI(TAG, "Fetching weather for location: %s (%.6f, %.6f)",
                 config.cityName, config.latitude, config.longitude);
        if (getGeneralWeatherFull(config.latitude, config.longitude, weather)) {
            TimingManager::markWeatherUpdated();
        } else {
            ESP_LOGE(TAG, "Failed to get weather information from DWD.");
        }
    } else {
        ESP_LOGI(TAG, "use cached Weather data, no data fetch needed");
    }
    printWeatherInfo(weather);
    DisplayManager::displayWeatherFull(weather);
}

void DeviceModeManager::updateDepartureFull() {
    // For departure-only mode, only check transport updates and active hours
    // Mode-specific data fetching and display
    DepartureData depart;

    // Fetch departure data only if needed and in active hours
    String stopIdToUse = String(config.selectedStopId);

    ESP_LOGI(TAG, "Fetching departures for stop: %s (%s)",
             stopIdToUse.c_str(), config.selectedStopName);

    if (getDepartureFromRMV(stopIdToUse.c_str(), depart)) {
        printTransportInfo(depart);
        TimingManager::markTransportUpdated();
         // Always display, even if empty
        if (depart.departureCount == 0) {
            ESP_LOGI(TAG, "No departures scheduled at this time");
        }
        DisplayManager::displayDeparturesFull(depart);
    } else {
        ESP_LOGE(TAG, "Failed to get departure information from RMV.");

        // Create empty departure data to show "No departures" message
        depart.stopId = stopIdToUse;
        depart.stopName = String(config.selectedStopName);
        depart.departureCount = 0;
        DisplayManager::displayDeparturesFull(depart);
    }
}

// ===== COMMON OPERATIONAL MODE FUNCTIONS =====

bool DeviceModeManager::setupConnectivityAndTime() {
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

            if (TimeManager::setupNTPTimeWithRetry(3)) {
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

// ===== HELPER FUNCTIONS FOR DATA FETCHING =====

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
            ESP_LOGI(TAG, "No departures found for stop - this is normal (empty schedule)");
            return true; // ✅ Empty list is valid - not an error
        }
    } else {
        ESP_LOGE(TAG, "Failed to get departure information from RMV");
        return false;
    }
}

// ===== CONFIGURATION PHASE MANAGEMENT =====

ConfigPhase DeviceModeManager::getCurrentPhase() {
    // Phase 1: WiFi not configured or credentials empty
    if (strlen(config.ssid) == 0) {
        ESP_LOGI(TAG, "Configuration Phase: 1 (WiFi Setup)");
        return PHASE_WIFI_SETUP;
    }

    if (config.displayMode == DISPLAY_MODE_WEATHER_ONLY && config.latitude != 0.0 && config.longitude != 0.0) {
        ESP_LOGI(TAG, "Configuration Phase: 3 (Complete - Weather Only Mode)");
        return PHASE_COMPLETE;
    }

    if (config.displayMode == DISPLAY_MODE_TRANSPORT_ONLY && strlen(config.selectedStopId) != 0) {
        ESP_LOGI(TAG, "Configuration Phase: 3 (Complete - Transport Only Mode)");
        return PHASE_COMPLETE;
    }

    if (config.displayMode == DISPLAY_MODE_HALF_AND_HALF && strlen(config.selectedStopId) != 0
        && config.latitude != 0.0
        && config.longitude != 0.0) {
        ESP_LOGI(TAG, "Configuration Phase: 3 (Complete - Half-and-Half Mode)");
        return PHASE_COMPLETE;
    }

    ESP_LOGI(TAG, "Configuration Phase: 2 (App Setup)");
    return PHASE_APP_SETUP;
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
