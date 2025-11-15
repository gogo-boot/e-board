#include "util/boot_flow_manager.h"
#include "util/device_mode_manager.h"
#include "util/wifi_manager.h"
#include "config/config_manager.h"
#include <esp_log.h>

static const char* TAG = "BOOT_FLOW";

namespace BootFlowManager {
    // Static references to shared components
    static WebServer* serverPtr = nullptr;
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* displayPtr = nullptr;
    static U8G2_FOR_ADAFRUIT_GFX* u8g2Ptr = nullptr;

    // RTC memory for persistent state across deep sleep
    RTC_DATA_ATTR static bool hasValidConfig = false;

    // Forward declarations for internal functions
    static void handlePhaseWifiSetup();
    static void handlePhaseAppSetup();
    static void handlePhaseComplete();
    static uint8_t determineDisplayMode(int8_t buttonMode);
    static void runOperationalMode(uint8_t displayMode);

    void initialize(WebServer& webServer,
                    GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& display,
                    U8G2_FOR_ADAFRUIT_GFX& u8g2) {
        serverPtr = &webServer;
        displayPtr = &display;
        u8g2Ptr = &u8g2;
        ESP_LOGI(TAG, "Boot flow manager initialized");
    }

    static void handlePhaseWifiSetup() {
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

        // // Start mDNS responder
        // if (MDNS.begin("mystation")) {
        //     ESP_LOGI(TAG, "mDNS started: http://mystation.local");
        // } else {
        //     ESP_LOGW(TAG, "mDNS failed to start");
        // }
    }

    static void handlePhaseAppSetup() {
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
            handlePhaseWifiSetup();
        }
    }

    static uint8_t determineDisplayMode(int8_t buttonMode) {
        RTCConfigData& config = ConfigManager::getConfig();

        // Button mode takes precedence over configured mode
        uint8_t displayMode = buttonMode >= 0 ? buttonMode : config.displayMode;

        ESP_LOGI(TAG, "Display mode: %d (button: %d, config: %d)",
                 displayMode, buttonMode, config.displayMode);

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

        case DISPLAY_MODE_DEPARTURE_ONLY:
            ESP_LOGI(TAG, "Starting Departure-only full screen mode");
            DeviceModeManager::updateDepartureFull();
            break;

        default:
            ESP_LOGW(TAG, "Unknown display mode %d, defaulting to half-and-half", displayMode);
            DeviceModeManager::showWeatherDeparture();
            break;
        }
    }

    static void handlePhaseComplete() {
        ESP_LOGI(TAG, "Phase 3: All configured - Running operational mode");

        // Verify configuration is still valid
        if (hasValidConfig || DeviceModeManager::hasValidConfiguration(hasValidConfig)) {
            // Get button mode (if device was woken by button press)
            // This is handled by ButtonManager::handleWakeupMode() before we get here
            RTCConfigData& config = ConfigManager::getConfig();
            int8_t buttonMode = config.inTemporaryMode ? config.temporaryDisplayMode : -1;

            // Determine and run operational mode
            uint8_t displayMode = determineDisplayMode(buttonMode);
            runOperationalMode(displayMode);

            // After operational mode completes, enter deep sleep
            DeviceModeManager::enterOperationalSleep();
        } else {
            // Configuration became invalid - go back to configuration
            ESP_LOGW(TAG, "Configuration validation failed - entering configuration mode");
            DeviceModeManager::runConfigurationMode();
        }
    }

    void handleBootFlow() {
        // Determine device mode based on saved configuration
        ConfigPhase phase = DeviceModeManager::getCurrentPhase();

        switch (phase) {
        case PHASE_WIFI_SETUP:
            handlePhaseWifiSetup();
            break;

        case PHASE_APP_SETUP:
            handlePhaseAppSetup();
            break;

        case PHASE_COMPLETE:
            handlePhaseComplete();
            break;

        default:
            ESP_LOGE(TAG, "Unknown phase: %d", phase);
            handlePhaseWifiSetup();
            break;
        }
    }
} // namespace BootFlowManager




