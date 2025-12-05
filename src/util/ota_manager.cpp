#include "util/ota_manager.h"
#include "config/config_manager.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "ota/ota_update.h"
#include <ctime>

static const char* TAG = "OTA_MANAGER";

namespace OTAManager {
    bool shouldCheckForUpdate() {
        RTCConfigData& config = ConfigManager::getConfig();

        // Check if OTA is enabled
        if (!config.otaEnabled) {
            ESP_LOGD(TAG, "OTA automatic updates are disabled");
            return false;
        }

        // Get current time
        tm timeinfo = {};
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
            ESP_LOGI(TAG, "OTA update time matched! Configured: %s, Current: %02d:%02d",
                     config.otaCheckTime, currentHour, currentMinute);
            return true;
        }

        ESP_LOGD(TAG, "OTA update time not matched. Configured: %s, Current: %02d:%02d",
                 config.otaCheckTime, currentHour, currentMinute);
        return false;
    }

    void checkAndApplyUpdate() {
        if (shouldCheckForUpdate()) {
            ESP_LOGI(TAG, "Starting OTA update check...");
            check_update_task(nullptr);

            // Mark OTA check timestamp to prevent repeated checks
            time_t now;
            time(&now);
            TimingManager::setLastOTACheck((uint32_t)now);

            // Note: If update is found and installed, device will restart
            // If no update or update fails, execution continues normally
        }
    }
} // namespace OTAManager

