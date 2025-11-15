#include "util/system_init.h"
#include "util/factory_reset.h"
#include "util/battery_manager.h"
#include "util/button_manager.h"
#include "config/config_manager.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <nvs_flash.h>

static const char* TAG = "SYSTEM_INIT";

namespace SystemInit {
    void printWakeupCause() {
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
            ESP_LOGI(TAG, "  → EXT1 (multiple GPIO RTC_IO)");
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
    }

    bool checkAndHandleFactoryReset() {
        if (FactoryReset::checkFactoryResetButton()) {
            nvs_flash_init();
            FactoryReset::performFactoryReset();
            return true;
        }
        return false;
    }

    void initialize() {
#if PRODUCTION > 0
        esp_log_level_set("*", ESP_LOG_ERROR);
#else
        Serial.begin(115200);
        delay(100);
        esp_log_level_set("*", ESP_LOG_DEBUG);
#endif

        ESP_LOGI(TAG, "System starting...");
        printWakeupCause();
        checkAndHandleFactoryReset();
        BatteryManager::init();
        ButtonManager::init();

        ConfigManager& configMgr = ConfigManager::getInstance();
        configMgr.loadFromNVS();
        ESP_LOGI(TAG, "Configuration loaded from NVS - wifiConfigured: %d",
                 ConfigManager::getConfig().wifiConfigured);
        ESP_LOGI(TAG, "System initialization complete");
    }
} // namespace SystemInit
