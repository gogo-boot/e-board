#include "display/common_footer.h"
#include "display/display_shared.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include <esp_log.h>
#include <icons.h>
#include <WiFi.h>

static const char* TAG = "COMMON_FOOTER";

void CommonFooter::drawFooter(int16_t x, int16_t y, int16_t h, uint8_t elements) {
    auto* display = DisplayShared::getDisplay();
    auto* u8g2 = DisplayShared::getU8G2();
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "Display not initialized! Call DisplayShared::init() first.");
        return;
    }
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
        drawRefreshIcon(currentX, y);
    }
    // Draw WiFi status if requested
    if (elements & FOOTER_WIFI) {
        drawWiFiStatus(currentX, footerY);
    }
    // Draw battery status if requested
    if (elements & FOOTER_BATTERY) {
        drawBatteryStatus(currentX, footerY);
    }
}

String CommonFooter::getTimeString() {
    String footerText = "";
    if (TimeManager::isTimeSet()) {
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.", &timeinfo);
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
    auto* display = DisplayShared::getDisplay();
    if (!display) return;
    // Get WiFi signal strength
    int32_t rssi = WiFi.RSSI();
    icon_name wifiIcon;
    if (WiFi.status() != WL_CONNECTED) {
        wifiIcon = wifi_off; // Assume we have a wifi_off icon
    } else if (rssi > -50) {
        wifiIcon = wifi; // Strong signal
    } else if (rssi > -60) {
        wifiIcon = wifi_3_bar; // Good signal
    } else if (rssi > -70) {
        wifiIcon = wifi_2_bar; // Fair signal
    } else {
        wifiIcon = wifi_1_bar; // Weak signal
    }
    display->drawInvertedBitmap(currentX, y, getBitmap(wifiIcon, 16), 16, 16, GxEPD_BLACK);
    currentX += 20; // Move right
}

void CommonFooter::drawBatteryStatus(int16_t& currentX, int16_t y) {
    auto* display = DisplayShared::getDisplay();
    if (!display) return;
    // TODO: Implement actual battery reading
    // For now, use a default battery icon
    icon_name batteryIcon = Battery_5; // Assume we have battery icons

    display->drawInvertedBitmap(currentX, y, getBitmap(batteryIcon, 16), 16, 16, GxEPD_BLACK);
    currentX += 20; // Move right
}

void CommonFooter::drawRefreshIcon(int16_t& currentX, int16_t y) {
    auto* display = DisplayShared::getDisplay();
    if (!display) return;

    display->drawInvertedBitmap(currentX, y, getBitmap(refresh, 16), 16, 16, GxEPD_BLACK);
    currentX += 20; // Move right
}
