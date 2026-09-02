#include "display/common_footer.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "util/battery_manager.h"
#include "util/timing_manager.h"
#include <icons.h>
#include <WiFi.h>
#include "global_instances.h"
#include "build_config.h"

static const char* TAG = "COMMON_FOOTER";

// Static member definitions — RTC-persistent so last-known WiFi state survives
// deep sleep and is available on wakes that skip the WiFi connect.
RTC_DATA_ATTR int32_t CommonFooter::cachedRSSI = 0;
RTC_DATA_ATTR bool CommonFooter::cachedConnected = false;

void CommonFooter::cacheWiFiState() {
    // Only refresh the cache while WiFi is actually up. On a wake that skipped
    // WiFi, this preserves the last-known-good state instead of overwriting it
    // with a disconnected/zero reading.
    if (WiFi.status() == WL_CONNECTED) {
        cachedRSSI = WiFi.RSSI();
        cachedConnected = true;
    }
}


void CommonFooter::drawFooter(int16_t x, int16_t y, int16_t h, uint8_t elements) {
    TextUtils::setFont10px_margin12px(); // Small font for footer
    int16_t footerY = y + h - 14; // Bottom of section
    int16_t currentX = x + 10; // Start position with margin
    // Draw time if requested
    if (elements & FOOTER_TIME) {
        String timeText = getTimeString();
        TextUtils::printTextAtWithMargin(currentX, footerY, timeText);
        currentX += TextUtils::getTextWidth(timeText) + 5; // Move right with spacing
    }
    // Draw refresh icon if requested
    if (elements & FOOTER_REFRESH) {
        drawRefreshIcon(currentX, footerY);
    }
    // Draw WiFi status if requested
    if (elements & FOOTER_WIFI) {
        drawWiFiStatus(currentX, footerY);
    }
    // Draw battery status if requested
    if (elements & FOOTER_BATTERY && SHOW_BATTERY_STATUS) {
        drawBatteryStatus(currentX, footerY);
        drawBatteryText(currentX, footerY);
    }

    DEBUG_ONLY(
        String sleepTime = "Sleep Time: " + String(TimingManager::getNextSleepDurationSeconds()) + "s";
        TextUtils::printTextAtWithMargin(currentX, footerY, sleepTime);
    );
}

String CommonFooter::getTimeString() {
    String footerText = "";
    if (TimeManager::isTimeSet()) {
        tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
            footerText = String(timeStr);
        } else {
            footerText = "Zeit nicht verfügbar";
        }
    } else {
        footerText = "Zeit nicht synchronisiert";
    }
    return footerText;
}

void CommonFooter::drawWiFiStatus(int16_t& currentX, int16_t y) {
    icon_name wifiIcon = getWiFiIcon();
    int16_t iconY = y - 7; // Center 24px icon with text baseline (adjust ±px if needed)
    display.drawInvertedBitmap(currentX, iconY, getBitmap(wifiIcon, 24), 24, 24, GxEPD_BLACK);
    currentX += 28; // Move right
}

icon_name CommonFooter::getWiFiIcon() {
    if (!cachedConnected) {
        return wifi_off;
    } else if (cachedRSSI > -60) {
        return wifi; // Strong signal
    } else if (cachedRSSI > -70) {
        return wifi_3_bar; // Good signal
    } else if (cachedRSSI > -80) {
        return wifi_2_bar; // Fair signal
    } else {
        return wifi_1_bar; // Weak signal
    }
}

void CommonFooter::drawBatteryStatus(int16_t& currentX, int16_t y) {
    // Check if battery monitoring is available
    if (!BatteryManager::isAvailable()) {
        ESP_LOGD(TAG, "Battery monitoring not available on this board");
        return;
    }

    // Get battery icon level (1-5)
    int iconLevel = BatteryManager::getBatteryIconLevel();
    if (iconLevel <= 0) {
        ESP_LOGW(TAG, "Unable to read battery status");
        return;
    }

    icon_name batteryIcon = getBatteryIcon();
    int16_t iconY = y - 7; // Center 24px icon with text baseline (adjust ±px if needed)
    display.drawInvertedBitmap(currentX, iconY, getBitmap(batteryIcon, 24), 24, 24, GxEPD_BLACK);
    currentX += 28; // Move right
}

void CommonFooter::drawBatteryText(int16_t& currentX, int16_t y) {
    // Log battery info for debugging
    int percentage = BatteryManager::getBatteryPercentage();

    ESP_LOGD(TAG, "Battery: %d%%", percentage);
    char batteryText[32];
    sprintf(batteryText, "%d%%", percentage);
    TextUtils::printTextAtWithMargin(currentX, y, String(batteryText));
    currentX += TextUtils::getTextWidth(String(batteryText)) + 5; // Move right with spaci
}

icon_name CommonFooter::getBatteryIcon() {
    // Check if battery monitoring is available
    if (!BatteryManager::isAvailable()) {
        return Battery_3; // Default fallback
    }

    // Get battery icon level (1-5)
    int iconLevel = BatteryManager::getBatteryIconLevel();
    if (iconLevel <= 0) {
        return Battery_3; // Default fallback
    }

    // Check if charging first
    if (BatteryManager::isCharging()) {
        return battery_charging_full_90deg;
    }

    // Map icon level to icon name
    icon_name batteryIcon;
    switch (iconLevel) {
    case 1: batteryIcon = Battery_1;
        break;
    case 2: batteryIcon = Battery_2;
        break;
    case 3: batteryIcon = Battery_3;
        break;
    case 4: batteryIcon = Battery_4;
        break;
    case 5: batteryIcon = Battery_5;
        break;
    default: batteryIcon = Battery_3;
        break; // Fallback
    }

    return batteryIcon;
}

void CommonFooter::drawRefreshIcon(int16_t& currentX, int16_t y) {
    int16_t iconY = y - 7; // Center 24px icon with text baseline (adjust ±px if needed)
    display.drawInvertedBitmap(currentX, iconY, getBitmap(refresh, 24), 24, 24, GxEPD_BLACK);
    currentX += 28; // Move right
}
