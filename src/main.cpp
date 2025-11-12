/*
 * MyStation E-Board - ESP32-C3 Public Transport Departure Board
 *
 * Boot Process Flow:
 * 1. System starts and calls DeviceModeManager::hasValidConfiguration()
 * 2. If no valid config -> DeviceModeManager::runConfigurationMode()
 *    - Creates WiFi hotspot for setup
 *    - Starts web server for user configuration
 *    - Stays awake to handle configuration
 * 3. If valid config exists -> DeviceModeManager::runOperationalMode()
 *    - Connects to saved WiFi
 *    - Fetches transport and weather data
 *    - Updates display and enters deep sleep
 *
 * Configuration persists in NVS (Non-Volatile Storage) across:
 * - Power loss/battery changes
 * - Firmware updates
 * - Manual resets
 */

#define ARDUINOJSON_DECODE_NESTING_LIMIT 1000
#include <Arduino.h>
#include <WebServer.h>
#include <esp_log.h>
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "util/device_mode_manager.h"
#include "util/battery_manager.h"
#include "util/button_manager.h"
#include "util/timing_manager.h"
#include "util/sleep_utils.h"
#include "display/display_manager.h"
#include "util/wifi_manager.h"
#include <SPI.h>
#include "ota/ota_update.h"
//EPD
#include "config/pins.h"

// GxEPD2 display library includes for GDEY075T7 (800x480)
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>  // Specific driver for GDEY075T7

#include "util/wifi_manager.h"
#include "util/time_manager.h"

// Create display instance for GDEY075T7 (800x480 resolution)
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
    GxEPD2_750_GDEY075T7(Pins::EPD_CS, Pins::EPD_DC, Pins::EPD_RES, Pins::EPD_BUSY));

// Create U8g2 instance for UTF-8 font support (for German umlauts)
U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char* TAG = "MAIN";

/**
 * Check if OTA update should run based on configuration
 * @return true if OTA is enabled and current time is within 1 minute of configured check time
 */
bool shouldRunOTAUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Check if OTA is enabled
    if (!config.otaEnabled) {
        ESP_LOGD(TAG, "OTA automatic updates are disabled");
        return false;
    }

    // Get current time
    struct tm timeinfo = {};
    if (!TimeManager::getCurrentLocalTime(timeinfo)) {
        ESP_LOGW(TAG, "Failed to get current time for OTA check");
        return false;
    }

    // Parse configured OTA check time (format: "HH:MM")
    int configHour = 0;
    int configMinute = 0;
    if (sscanf(config.otaCheckTime, "%d:%d", &configHour, &configMinute) != 2) {
        ESP_LOGW(TAG, "Invalid OTA check time format: %s", config.otaCheckTime);
        return false;
    }

    // Get current time
    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;

    // Check if current time is within 1 minute tolerance of configured time
    bool isTimeMatch = (currentHour == configHour &&
        abs(currentMinute - configMinute) <= 1);

    if (isTimeMatch) {
        ESP_LOGI(TAG, "OTA update time matched! Configured: %s, Current: %02d:%02d", config.otaCheckTime, currentHour,
                 currentMinute);
        return true;
    }

    ESP_LOGD(TAG, "OTA update time not matched. Configured: %s, Current: %02d:%02d",
             config.otaCheckTime, currentHour, currentMinute);
    return false;
}

// --- Globals (shared across modules) ---
WebServer server(80);
RTC_DATA_ATTR unsigned long loopCount = 0;
RTC_DATA_ATTR bool hasValidConfig = false; // Flag to track if valid config exists


void setup() {
#if PRODUCTION > 0
    // Production: Only critical errors
    esp_log_level_set("*", ESP_LOG_ERROR);
#else
    Serial.begin(115200);
    delay(5000);
    // Development: Full logging
    esp_log_level_set("*", ESP_LOG_DEBUG);
#endif
    // Allow time for serial monitor to connect, only for local debugging, todo remove in production or activate by flag

    // esp_log_level_set("*", ESP_LOG_DEBUG); // Set global log level
    ESP_LOGI(TAG, "System starting...");

    // DIAGNOSTIC: Print wakeup cause
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "=== Wakeup Cause: %d ===", wakeup_reason);
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        ESP_LOGI(TAG, "  → Power on or reset (not from deep sleep)");
        break;
    case ESP_SLEEP_WAKEUP_EXT0:
        ESP_LOGI(TAG, "  → EXT0 (single GPIO RTC_IO)");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "  → EXT1 (multiple GPIO RTC_CNTL)");
        ESP_LOGI(TAG, "  → Wakeup pin mask: 0x%llx", esp_sleep_get_ext1_wakeup_status());
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "  → Timer wakeup");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        ESP_LOGI(TAG, "  → Touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        ESP_LOGI(TAG, "  → ULP program");
        break;
    default:
        ESP_LOGI(TAG, "  → Unknown: %d", wakeup_reason);
        break;
    }

    // Initialize battery manager (only available on ESP32-S3)
    BatteryManager::init();

    // Initialize button manager (ESP32-S3 only)
    ButtonManager::init();

    // Check if device was woken by button press (temporary display mode)
    // int8_t buttonMode = ButtonManager::getWakeupButtonMode();
    // if (buttonMode >= 0) {
    //     ESP_LOGI(TAG, "Woken by button press! Temporary display mode: %d", buttonMode);
    //
    //     // Store temporary mode information in RTC memory
    //     RTCConfigData& config = ConfigManager::getConfig();
    //     config.inTemporaryMode = true;
    //     config.temporaryDisplayMode = buttonMode;
    //
    //     // Sleep for 2 minutes with button wakeup enabled
    //     ESP_LOGI(TAG, "Entering 2-minute sleep for temporary mode (buttons still active)");
    //
    //     // enterDeepSleepWithButtonWakeup(ButtonManager::TEMPORARY_MODE_TIMEOUT);
    //     // Device will restart after wakeup
    // }
    //
    // Check if OTA update should run based on configuration
    if (shouldRunOTAUpdate()) {
        ESP_LOGI(TAG, "Starting OTA update check...");
        check_update_task(nullptr);

        // Mark OTA check timestamp to prevent repeated checks
        time_t now;
        time(&now);
        TimingManager::setLastOTACheck((uint32_t)now);
        // Note: If update is found and installed, device will restart
        // If no update or update fails, execution continues normally
    }

    // Check if device was woken by button press (temporary display mode)
    int8_t buttonMode = ButtonManager::getWakeupButtonMode();
    if (buttonMode >= 0) {
        ESP_LOGI(TAG, "Woken by button press! Temporary display mode: %d", buttonMode);

        // Store temporary mode information in RTC memory
        RTCConfigData& config = ConfigManager::getConfig();
        config.inTemporaryMode = true;
        config.temporaryDisplayMode = buttonMode;
    }
    // Determine device mode based on saved configuration
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();

    switch (phase) {
    case PHASE_WIFI_SETUP:
        ESP_LOGI(TAG, "Phase 1: WiFi Setup Required");
        DeviceModeManager::showPhaseInstructions(PHASE_WIFI_SETUP);
        DeviceModeManager::runConfigurationMode();
        break;

    case PHASE_APP_SETUP: {
        ESP_LOGI(TAG, "Phase 2: Application Setup Required");

        // Verify WiFi still works and has internet before proceeding
        RTCConfigData& config = ConfigManager::getConfig();

        // Try to connect with stored credentials
        MyWiFiManager::reconnectWiFi();

        if (MyWiFiManager::isConnected() && MyWiFiManager::hasInternetAccess()) {
            DeviceModeManager::showPhaseInstructions(PHASE_APP_SETUP);
            DeviceModeManager::runConfigurationMode();
        } else {
            // WiFi/Internet connection failed - revert to Phase 1
            ESP_LOGE(TAG, "WiFi validation failed - reverting to Phase 1");
            config.wifiConfigured = false;
            ConfigManager& configMgr = ConfigManager::getInstance();
            configMgr.saveToNVS();
            DeviceModeManager::showWifiErrorPage();
            DeviceModeManager::showPhaseInstructions(PHASE_WIFI_SETUP);
            DeviceModeManager::runConfigurationMode();
        }
    }
    break;

    case PHASE_COMPLETE: {
        ESP_LOGI(TAG, "Phase 3: All configured - Running operational mode");

        // Verify WiFi still works before entering operational mode
        if (hasValidConfig || DeviceModeManager::hasValidConfiguration(hasValidConfig)) {
            // Get configured display mode from NVS/RTC
            RTCConfigData& config = ConfigManager::getConfig();
            uint8_t displayMode = buttonMode >= 0 ? buttonMode : config.displayMode;

            ESP_LOGI(TAG, "Operating in configured display mode: %d", displayMode);

            // Run operational mode based on configured display mode
            switch (displayMode) {
            case DISPLAY_MODE_HALF_AND_HALF:
                ESP_LOGI(TAG, "Starting Weather + Departure half-and-half mode");
                DeviceModeManager::showWeatherDeparture();
                break;

            case DISPLAY_MODE_WEATHER_ONLY:
                ESP_LOGI(TAG, "Starting Weather-only full screen mode");
                DeviceModeManager::updateWeatherFull();
                break;

            case DISPLAY_MODE_DEPARTURE_ONLY:
                ESP_LOGI(TAG, "Starting Departure-only full screen mode");
                DeviceModeManager::updateDepartureFull();
                break;
            default:
                ESP_LOGW(TAG, "Unknown display mode %d, defaulting to half-and-half", displayMode);
                DeviceModeManager::showWeatherDeparture();
                break;
            }

            // After operational mode completes, enter deep sleep
            DeviceModeManager::enterOperationalSleep();
        } else {
            // Configuration became invalid - go back to configuration
            DeviceModeManager::runConfigurationMode();
        }
    }
    break;
    }
}

void loop() {
    // Only handle web server in config mode
    if (ConfigManager::isConfigMode()) {
        server.handleClient();
        delay(10); // Small delay to prevent watchdog issues
    } else {
        // Normal operation happens in setup() and then device goes to sleep
        // This should never be reached in normal operation
        ESP_LOGW(TAG, "Unexpected: loop() called in normal operation mode");
        delay(5000);
    }
}
