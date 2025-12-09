#include "util/boot_flow_manager.h"
#include "util/device_mode_manager.h"
#include "util/wifi_manager.h"
#include "config/config_manager.h"
#include <WebServer.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "util/timing_manager.h"

static const char* TAG = "BOOT_FLOW";

namespace BootFlowManager {
    // RTC memory for persistent state across deep sleep
    RTC_DATA_ATTR static bool hasValidConfig = false;

    // Forward declarations for internal functions
    static uint8_t determineDisplayMode(int8_t buttonMode);
    static void runOperationalMode(uint8_t displayMode);

    void handlePhaseWifiSetup() {
        ESP_LOGI(TAG, "==========================================");
        ESP_LOGI(TAG, "=== PHASE 1: WiFi Setup ===");
        ESP_LOGI(TAG, "==========================================");

        // Show setup instructions on display
        DeviceModeManager::showPhaseInstructions(PHASE_WIFI_SETUP);

        // Initialize configuration with defaults
        ConfigManager::setDefaults();

        // Attempt WiFi setup
        WiFiManager wm;
        MyWiFiManager::setupWiFiAccessPointAndRestart(wm);
    }

    void handlePhaseAppSetup() {
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
            DeviceModeManager::showWifiErrorPage();
            handlePhaseWifiSetup();
        }
    }

    static uint8_t determineDisplayMode(int8_t buttonMode) {
        RTCConfigData& config = ConfigManager::getConfig();

        // Button mode takes precedence over configured mode
        if (buttonMode >= 0) {
            return buttonMode;
        }

        int8_t displayMode = config.displayMode;
        if (config.displayMode == DISPLAY_MODE_HALF_AND_HALF) {
            if (TimingManager::isTransportActiveTime()) {
                displayMode = DISPLAY_MODE_HALF_AND_HALF;
            } else {
                displayMode = DISPLAY_MODE_WEATHER_ONLY;
            }
        }

        return displayMode;
    }

    static void runOperationalMode(uint8_t displayMode) {
        switch (displayMode) {
        case DISPLAY_MODE_HALF_AND_HALF:
            ESP_LOGI(TAG, "Starting Weather + Departure half-and-half mode");
            DeviceModeManager::showWeatherDeparture();
            break;

        case DISPLAY_MODE_WEATHER_ONLY:
            ESP_LOGI(TAG, "Starting Weather-only full screen mode");
            DeviceModeManager::updateWeatherFull();
            break;

        case DISPLAY_MODE_TRANSPORT_ONLY:
            ESP_LOGI(TAG, "Starting Departure-only full screen mode");
            DeviceModeManager::updateDepartureFull();
            break;

        default:
            ESP_LOGW(TAG, "Unknown display mode %d, defaulting to half-and-half", displayMode);
            DeviceModeManager::showWeatherDeparture();
            break;
        }
    }

    void handlePhaseComplete() {
        ESP_LOGI(TAG, "Phase 3: All configured - Running operational mode");

        // Get button mode (if device was woken by button press)
        // This is handled by ButtonManager::handleWakeupMode() before we get here
        RTCConfigData& config = ConfigManager::getConfig();

        // DEBUG: Log temp mode state on wake
        ESP_LOGI(TAG, "=== TEMP MODE DEBUG ON WAKE ===");
        ESP_LOGI(TAG, "Wakeup cause: %d", esp_sleep_get_wakeup_cause());
        ESP_LOGI(TAG, "inTemporaryMode: %d", config.inTemporaryMode);
        ESP_LOGI(TAG, "temporaryDisplayMode: %d", config.temporaryDisplayMode);
        ESP_LOGI(TAG, "temporaryModeActivationTime: %u", config.temporaryModeActivationTime);
        ESP_LOGI(TAG, "Configured displayMode: %d", config.displayMode);
        ESP_LOGI(TAG, "===============================");

        int8_t buttonMode = config.inTemporaryMode ? config.temporaryDisplayMode : -1;

        // Determine and run operational mode
        uint8_t displayMode = determineDisplayMode(buttonMode);

        ESP_LOGI(TAG, "Display mode determined: %d (buttonMode=%d, configured=%d)",
                 displayMode, buttonMode, config.displayMode);

        runOperationalMode(displayMode);
    }
} // namespace BootFlowManager




