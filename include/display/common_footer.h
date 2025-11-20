#pragma once
#include <Arduino.h>
#include <icons.h>

enum FooterElements {
    FOOTER_TIME = 1 << 0, // Show current time
    FOOTER_REFRESH = 1 << 1, // Show refresh icon
    FOOTER_WIFI = 1 << 2, // Show WiFi signal strength
    FOOTER_BATTERY = 1 << 3, // Show battery status
    FOOTER_ALL = FOOTER_TIME | FOOTER_REFRESH | FOOTER_WIFI | FOOTER_BATTERY
};

class CommonFooter {
public:
    // Draw footer with specified elements
    // elements: bitwise OR of FooterElements flags
    static void drawFooter(int16_t x, int16_t y, int16_t h, uint8_t elements = FOOTER_TIME | FOOTER_REFRESH);

    // Helper functions to get icon names (for reuse in other display contexts)
    static icon_name getWiFiIcon();
    static icon_name getBatteryIcon();

private:
    static String getTimeString();
    static void drawWiFiStatus(int16_t& currentX, int16_t y);
    static void drawBatteryStatus(int16_t& currentX, int16_t y);
    static void drawRefreshIcon(int16_t& currentX, int16_t y);
};
