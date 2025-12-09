#include "util/system_init.h"
#include "util/factory_reset.h"
#include "config/config_manager.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <nvs_flash.h>

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

// Font includes for German character support
#include <U8g2_for_Adafruit_GFX.h>

#include "display/display_manager.h"

static const char* TAG = "SYSTEM_INIT";

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

namespace SystemInit {
    void initSerialConnector() {
#if PRODUCTION > 0
        esp_log_level_set("*", ESP_LOG_ERROR);
#else
        Serial.begin(115200);
        delay(5000);
        esp_log_level_set("*", ESP_LOG_DEBUG);
#endif
    }

    void factoryResetIfDesired() {
        if (FactoryReset::checkFactoryResetButton()) {
            nvs_flash_init();
            FactoryReset::performFactoryReset();
        }
    }

    void initDisplay() {
        // Info : initial Parameter can be used to preserve screen content for partial updates
        display.init(DisplayConstants::SERIAL_BAUD_RATE, true,
                     DisplayConstants::RESET_DURATION_MS, false);
        // Landscape orientation
        display.setRotation(0);
    }

    void initFont() {
        // Initialize U8g2 for UTF-8 font support (German umlauts)
        u8g2.begin(display);
        u8g2.setFontMode(1); // Use u8g2 transparent mode
        u8g2.setFontDirection(0); // Left to right
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);
    }

    void loadNvsConfig() {
        // Restore RTCConfigData from NVS
        ConfigManager& configMgr = ConfigManager::getInstance();
        configMgr.loadFromNVS();
        ESP_LOGI(TAG, "Configuration loaded from NVS - wifiConfigured: %d",
                 ConfigManager::getConfig().wifiConfigured);
        ESP_LOGI(TAG, "System initialization complete");
    }
} // namespace SystemInit
