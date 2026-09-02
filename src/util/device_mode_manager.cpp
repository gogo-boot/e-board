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
#include "display/trip_display.h"
#include "util/battery_manager.h"
#include "util/transport_print.h"
#include "global_instances.h"

#include "util/sleep_utils.h"
#include "util/indoor_sensor.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "util/weather_print.h"
#include "util/wifi_manager.h"
#include "display/common_footer.h"

static const char* TAG = "DEVICE_MODE";

// Turn off WiFi before display rendering to save power (~100mA)
static void shutdownWiFiBeforeRender() {
    CommonFooter::cacheWiFiState();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    ESP_LOGI(TAG, "WiFi disabled before display rendering");
}

// Global variables needed for operation

ConfigManager& configMgr = ConfigManager::getInstance();
RTCConfigData& config = ConfigManager::getConfig();
RTC_DATA_ATTR WeatherInfo weather;

// Day-browse hourly cache, prefetched during each weather update so button
// presses render instantly without a WiFi round-trip. ~2 KB of RTC memory.
RTC_DATA_ATTR DayBrowsePoint dayCache[DAY_CACHE_MAX_DAYS][DAY_CACHE_HOURS];
RTC_DATA_ATTR uint8_t dayCacheValidDays = 0;  // contiguous valid days from day 0

void DeviceModeManager::runConfigurationMode() {
    ESP_LOGI(TAG, "=== PHASE 2: CONFIGURATION MODE ===");

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

    ESP_LOGI(TAG, "Configuration data ready. Web server will be started after display update.");
}

void DeviceModeManager::startWebServer() {
    // Start web server for configuration - called AFTER display has been updated
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
}

void DeviceModeManager::showApplicationInfo() {
    ESP_LOGI(TAG, "Showing application info screen");

    float voltage = BatteryManager::getBatteryVoltage();
    int percent = BatteryManager::getBatteryPercentage();

    shutdownWiFiBeforeRender();
    DisplayManager::displayApplicationInfo(voltage, percent);
}

void DeviceModeManager::showWeatherDeparture() {
    // Path: Outside active time -> Check if time to update weather
    bool needsWeatherUpdate = TimingManager::isTimeForWeatherUpdate();
    ESP_LOGI(TAG, "Update requirements - Weather: %s", needsWeatherUpdate ? "YES" : "NO");

    if (needsWeatherUpdate && getGeneralWeatherFull(config.latitude, config.longitude, weather)) {
        printWeatherInfo(weather);
        TimingManager::markWeatherUpdated();
    }

    if (config.tripMode) {
        // Trip/connection mode
        static TripData trip; // static: ~4KB too large for stack
        memset(&trip, 0, sizeof(trip));
        getTripFromRMV(config.selectedStopId, config.tripDestId, trip);
        TimingManager::markTransportUpdated();
        shutdownWiFiBeforeRender();
        DisplayManager::displayHalfNHalfTrip(weather, trip);
    } else {
        // Departure mode
        DepartureData depart;
        fetchTransportData(depart);
        TimingManager::markTransportUpdated();
        shutdownWiFiBeforeRender();
        DisplayManager::displayHalfNHalf(weather, depart);
    }
}

// Render a cached forecast day (06:00–23:00, up to 18 points) from the RTC
// day cache. Returns true if rendered; false if the day is not available in
// the cache (caller should fall back to the normal weather view).
static bool renderDayBrowseFromCache(int day) {
    if (day <= 0 || day >= dayCacheValidDays) {
        return false;
    }

    // Slice hours 06:00–23:00 from the cached day into the display struct.
    // The graph derives x-axis labels from an ISO timestamp via substring(11,16),
    // so build a full "YYYY-MM-DDTHH:00" string using the day's date prefix.
    char datePrefix[11] = {0};  // "YYYY-MM-DD"
    strncpy(datePrefix, weather.dailyForecast[day].time, 10);

    WeatherHourlyForecast dayHourly[DAY_BROWSE_HOURLY_COUNT];
    int count = 0;
    for (int h = DAY_BROWSE_START_HOUR; h < DAY_CACHE_HOURS; ++h) {
        const DayBrowsePoint& p = dayCache[day][h];
        WeatherHourlyForecast& out = dayHourly[count];
        snprintf(out.time, TIME_STRING_LENGTH, "%sT%02d:00", datePrefix, h);
        out.temperature = p.temperature;
        out.weatherCode = p.weatherCode;
        out.rainChance  = p.rainChance;
        out.rainfall    = p.rainfall;
        out.humidity    = p.humidity;
        count++;
    }

    // Append the 24:00 point (next day's 00:00) so the graph spans 18 intervals
    // that divide evenly by 3 for aligned 3-hour grid lines. Only possible when
    // the next day is cached; the last browsable day falls back to 18 points.
    if (day + 1 < dayCacheValidDays) {
        const DayBrowsePoint& p = dayCache[day + 1][0];
        WeatherHourlyForecast& out = dayHourly[count];
        // Label as "24:00" rather than the next day's 00:00 for a clear single-day
        // axis. This string is only string-sliced for display, never date-parsed.
        snprintf(out.time, TIME_STRING_LENGTH, "%sT24:00", datePrefix);
        out.temperature = p.temperature;
        out.weatherCode = p.weatherCode;
        out.rainChance  = p.rainChance;
        out.rainfall    = p.rainfall;
        out.humidity    = p.humidity;
        count++;
    }

    DisplayManager::displayWeatherDayBrowse(weather, dayHourly, count, day);
    return true;
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
            // Update available forecast days for day browsing button wrapping
            config.availableForecastDays = weather.dailyForecastCount;

            // Prefetch all forecast days' hourly data into the RTC cache while
            // WiFi is still up. Subsequent button presses render from cache with
            // no network delay. On failure the previous cache is left intact.
            int cached = getWeatherHourlyMultiDay(config.latitude, config.longitude,
                                                  weather.dailyForecastCount, dayCache);
            if (cached > 0) {
                dayCacheValidDays = (uint8_t)cached;
            } else {
                ESP_LOGW(TAG, "Day cache prefetch failed; keeping previous cache (%d days)",
                         dayCacheValidDays);
            }
        } else {
            ESP_LOGE(TAG, "Failed to get weather information from DWD.");
        }
    } else {
        ESP_LOGI(TAG, "use cached Weather data, no data fetch needed");
    }
    printWeatherInfo(weather);

    // Clamp selectedForecastDay to available range (model may have fewer days)
    if (config.selectedForecastDay >= weather.dailyForecastCount) {
        ESP_LOGW(TAG, "selectedForecastDay %d exceeds available %d, clamping to last day",
                 config.selectedForecastDay, weather.dailyForecastCount);
        config.selectedForecastDay = weather.dailyForecastCount - 1;
    }

    // WiFi is no longer needed — day browsing renders from the RTC cache.
    shutdownWiFiBeforeRender();

    // Day browsing: render the selected future day from cache. Falls back to the
    // normal today view if the day is not cached (cold boot before first fetch,
    // prefetch failure, or a model with fewer days).
    if (config.selectedForecastDay > 0) {
        if (renderDayBrowseFromCache(config.selectedForecastDay)) {
            return;
        }
        ESP_LOGW(TAG, "Day %d not in cache (valid=%d), falling back to today",
                 config.selectedForecastDay, dayCacheValidDays);
    }

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
        shutdownWiFiBeforeRender();
        DisplayManager::displayDeparturesFull(depart);
    } else {
        ESP_LOGE(TAG, "Failed to get departure information from RMV.");

        // Create empty departure data to show "No departures" message
        depart.stopId = stopIdToUse;
        depart.stopName = String(config.selectedStopName);
        depart.departureCount = 0;
        shutdownWiFiBeforeRender();
        DisplayManager::displayDeparturesFull(depart);
    }
}

// ===== COMMON OPERATIONAL MODE FUNCTIONS =====

bool DeviceModeManager::setupConnectivityAndTime() {
    if (MyWiFiManager::isConnected()) {
        bool timeIsSet = TimeManager::isTimeSet();
        bool needsSync = TimeManager::needsPeriodicSync();

        if (!timeIsSet) {
            // Time is not set at all — force NTP sync (first boot or power loss)
            ESP_LOGI(TAG, "Time not set, performing initial NTP synchronization...");
            if (TimeManager::setupNTPTimeWithRetry(3)) {
                ESP_LOGI(TAG, "Initial NTP sync successful");
            } else {
                ESP_LOGE(TAG, "Failed to sync time via NTP");
                return false; // Cannot proceed without time
            }
        } else if (needsSync) {
            // Time valid but 24h+ since last sync — correct RTC drift
            ESP_LOGI(TAG, "Periodic NTP sync due...");
            if (TimeManager::setupNTPTimeWithRetry(3)) {
                ESP_LOGI(TAG, "Periodic NTP sync successful");
            } else {
                ESP_LOGW(TAG, "Periodic NTP sync failed - continuing with RTC time");
            }
        }

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

    if (config.displayMode == DISPLAY_MODE_APPLICATION_INFO) {
        ESP_LOGI(TAG, "Configuration Phase: 3 (Complete - Application Info Mode)");
        return PHASE_COMPLETE;
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

void DeviceModeManager::logWifiError() {
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

// ===== PHASE HANDLERS (merged from BootFlowManager) =====

void DeviceModeManager::runOperationalMode(uint8_t displayMode) {
    switch (displayMode) {
    case DISPLAY_MODE_HALF_AND_HALF:
        ESP_LOGI(TAG, "Starting Weather + Departure half-and-half mode");
        showWeatherDeparture();
        break;
    case DISPLAY_MODE_WEATHER_ONLY:
        ESP_LOGI(TAG, "Starting Weather-only full screen mode");
        updateWeatherFull();
        break;
    case DISPLAY_MODE_TRANSPORT_ONLY:
        ESP_LOGI(TAG, "Starting Departure-only full screen mode");
        updateDepartureFull();
        break;
    case DISPLAY_MODE_APPLICATION_INFO:
        ESP_LOGI(TAG, "Starting Application Info mode");
        showApplicationInfo();
        break;
    default:
        ESP_LOGW(TAG, "Unknown display mode %d, defaulting to half-and-half", displayMode);
        showWeatherDeparture();
        break;
    }
}

void DeviceModeManager::handlePhaseWifiSetup() {
    ESP_LOGI(TAG, "=== PHASE 1: WiFi Setup ===");

    showPhaseInstructions(PHASE_WIFI_SETUP);
    ConfigManager::setDefaults();

    WiFiManager wm;
    MyWiFiManager::setupWiFiAccessPointAndRestart(wm);
}

void DeviceModeManager::handlePhaseAppSetup() {
    ESP_LOGI(TAG, "Phase 2: Application Setup Required");

    // Show instructions on e-paper and start web server immediately.
    // Location detection and nearby stops are loaded lazily via /api/init from the browser.
    showPhaseInstructions(PHASE_APP_SETUP);
    startWebServer();
}

void DeviceModeManager::handlePhaseComplete() {
    ESP_LOGI(TAG, "Phase 3: Running operational mode");

    RTCConfigData& cfg = ConfigManager::getConfig();

    // Sanitize APPLICATION_INFO if it leaked into persistent config
    if (cfg.displayMode == DISPLAY_MODE_APPLICATION_INFO) {
        ESP_LOGW(TAG, "config.displayMode is APPLICATION_INFO — resetting to WEATHER_ONLY");
        cfg.displayMode = DISPLAY_MODE_WEATHER_ONLY;
        ConfigManager::getInstance().saveToNVS();
    }

    uint8_t displayMode = TimingManager::getEffectiveDisplayMode();
    ESP_LOGI(TAG, "Display mode: %d (temp=%d, configured=%d)",
             displayMode, cfg.inTemporaryMode, cfg.displayMode);

#ifdef BOARD_S3_E1001
    IndoorSensor::init();
    IndoorSensor::read();
#endif

    runOperationalMode(displayMode);
}
