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

    // Cache WiFi state before turning WiFi off (call before WiFi.disconnect())
    static void cacheWiFiState();

    // Get cached RSSI value (valid after cacheWiFiState() is called)
    static int32_t getCachedRSSI() { return cachedRSSI; }

    // Helper functions to get icon names (for reuse in other display contexts)
    static icon_name getWiFiIcon();
    static icon_name getBatteryIcon();

private:
    // RTC-persistent so the last-known WiFi state survives deep sleep. This lets
    // wakes that skip WiFi (e.g. day-browse button wake rendering from cache)
    // still show the correct signal/connected state in the footer.
    static int32_t cachedRSSI;
    static bool cachedConnected;

    static String getTimeString();
    static void drawWiFiStatus(int16_t& currentX, int16_t y);
    static void drawBatteryStatus(int16_t& currentX, int16_t y);
    static void drawBatteryText(int16_t& currentX, int16_t y);
    static void drawRefreshIcon(int16_t& currentX, int16_t y);
};
