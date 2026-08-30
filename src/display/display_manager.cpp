#include <Arduino.h>
#include "display/display_manager.h"

#include "build_config.h"
#include "config/config_manager.h"
#include "display/transport_display.h"
#include "display/weather_general_half.h"
#include "display/weather_general_full.h"
#include "display/common_footer.h"
#include "display/trip_display.h"
#include "display/qr_code_helper.h"
#include "util/util.h"

#include "WiFiManager.h"

// Include bitmap icons
#include "icons.h"

#include "global_instances.h"

static const char* TAG = "DISPLAY_MGR";

// ===== STATIC MEMBER VARIABLES =====

int16_t DisplayManager::screenWidth = display.width(); // Will be read from display
int16_t DisplayManager::screenHeight = display.height(); // Will be read from display
int16_t DisplayManager::halfWidth = display.width() / 2; // Will be calculated
int16_t DisplayManager::halfHeight = display.height() / 2; // Will be calculated

// ===== INITIALIZATION METHODS =====

void DisplayManager::calculateDimensions() {
    // Landscape mode: 800x480 - split WIDTH in half
    // Weather: left half (0-399), Departures: right half (400-799)
    halfWidth = screenWidth / 2; // Split width: 400 pixels each
    halfHeight = screenHeight; // Full height: 480 pixels
    ESP_LOGI(TAG, "Landscape split: Weather[0,0,%d,%d] Departures[%d,0,%d,%d]",
             halfWidth, screenHeight, halfWidth, halfWidth, screenHeight);
}

// ===== DISPLAY UPDATE METHODS FOR EACH CASE =====

void DisplayManager::displayHalfNHalf(const WeatherInfo& weather,
                                      const DepartureData& departures) {
    ESP_LOGI(TAG, "Full update - both halves");

    const int16_t contentY = 0; // Start from top (no header)

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw both halves
        updateWeatherHalf(weather);
        updateDepartureHalf(departures);

        // Draw vertical divider
        displayVerticalLine(contentY);
    } while (display.nextPage());
}

void DisplayManager::displayHalfNHalfTrip(const WeatherInfo& weather, const TripData& tripData) {
    ESP_LOGI(TAG, "Full update - weather + trip connections");

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        updateWeatherHalf(weather);
        TripDisplay::drawTripConnections(tripData, halfWidth + 1, 0, screenWidth - halfWidth - 1, screenHeight);
        displayVerticalLine(0);
    } while (display.nextPage());
}

void DisplayManager::displayVerticalLine(const int16_t contentY) {
    display.drawLine(halfWidth, contentY, halfWidth, screenHeight, GxEPD_BLACK);
}

// ===== HALF-CONTENT UPDATE METHODS =====

void DisplayManager::updateWeatherHalf(const WeatherInfo& weather) {
    ESP_LOGI(TAG, "Updating weather half");

    const int16_t contentHeight = screenHeight; // Full height

    // Landscape: weather is LEFT half (full height)
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = halfWidth;

    int16_t leftMargin = x + DisplayConstants::MARGIN_HORIZONTAL;
    int16_t rightMargin = x + w - DisplayConstants::MARGIN_HORIZONTAL;

    // Draw weather layout
    WeatherHalfDisplay::drawHalfScreenWeatherLayout(weather, leftMargin, rightMargin, y, contentHeight);
    WeatherHalfDisplay::drawWeatherFooter(x, screenHeight - DisplayConstants::FOOTER_HEIGHT,
                                          DisplayConstants::FOOTER_HEIGHT);
}

void DisplayManager::updateDepartureHalf(const DepartureData& departures) {
    ESP_LOGI(TAG, "Updating departure half");

    const int16_t contentY = 0; // Start from top (no header)

    // Draw departure section
    TransportDisplay::drawHalfScreenTransportSection(
        departures, halfWidth, contentY, halfWidth, screenHeight);
}

// ===== FULL SCREEN DISPLAY METHODS =====

void DisplayManager::displayWeatherFull(const WeatherInfo& weather) {
    ESP_LOGI(TAG, "Displaying weather only mode");

    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        WeatherFullDisplay::drawFullScreenWeatherLayout(weather);
        WeatherFullDisplay::drawWeatherFooter(0, screenHeight - DisplayConstants::FOOTER_HEIGHT,
                                              DisplayConstants::FOOTER_HEIGHT);
    } while (display.nextPage());
}

void DisplayManager::displayWeatherDayBrowse(const WeatherInfo& weather,
                                              const WeatherHourlyForecast dayHourly[],
                                              int hourlyCount, int selectedDay) {
    ESP_LOGI(TAG, "Displaying weather day browse mode (day %d)", selectedDay);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        WeatherFullDisplay::drawDayBrowseLayout(weather, dayHourly, hourlyCount, selectedDay);
        WeatherFullDisplay::drawWeatherFooter(0, screenHeight - DisplayConstants::FOOTER_HEIGHT,
                                              DisplayConstants::FOOTER_HEIGHT);
    } while (display.nextPage());
}

void DisplayManager::displayDeparturesFull(const DepartureData& departures) {
    ESP_LOGI(TAG, "Displaying transports only mode");

    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        TransportDisplay::drawFullScreenTransportSection(departures, 0, 0,
                                                         screenWidth, screenHeight);
    } while (display.nextPage());
}

// ===== POWER MANAGEMENT =====

void DisplayManager::hibernate() {
    ESP_LOGI(TAG, "Hibernating display");

    // Turn off display
    display.hibernate();

    // You can add additional power-saving measures here
    ESP_LOGI(TAG, "Display hibernated");
}


// ===== CONFIGURATION MODE DISPLAY =====

void DisplayManager::displayPhase1WifiSetup() {
    ESP_LOGI(TAG, "=== DISPLAYING PHASE 1: WIFI SETUP (GERMAN) ===");

    // Get dynamic SSID with hardware ID
    String apSSID = Util::getUniqueSSID("MyStation");
    ESP_LOGI(TAG, "AP SSID: %s", apSSID.c_str());

    // Prepare QR code data
    String wifiQR = "WIFI:S:" + apSSID + ";;"; // WiFi connection string (no password)
    String urlQR = "http://10.0.1.1"; // Captive portal URL

    // Start display update
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Set up fonts
        u8g2.setFont(u8g2_font_helvB18_tf); // Bold 18pt for title
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        int16_t y = 40; // Start position from top
        const int16_t lineHeight = 35; // Spacing between lines
        const int16_t margin = 20; // Left margin
        const int16_t instructionWidth = 500; // Width for instructions (left side)

        // Draw title
        u8g2.setCursor(margin, y);
        u8g2.print("EINRICHTUNG 1/2 : MyStation mit Ihrem WLAN verbinden");

        // Draw underline for title
        display.drawFastHLine(margin, y + 5, screenWidth - (2 * margin), GxEPD_BLACK);

        y += lineHeight + 5;

        // Draw instruction lines in German
        u8g2.setFont(u8g2_font_helvB10_tf);

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("1. Gerät einschalten (Schalter auf ON schieben)");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("2. Mit dem MyStation-WLAN verbinden:");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("- QR-Code 1 scannen (verbindet automatisch)");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("- ODER manuell: WLAN \"" + apSSID + "\" wählen (kein Passwort)");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("3. Einrichtungsseite öffnen:");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("- Seite öffnet sich automatisch");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("- Falls nicht: QR-Code 2 scannen oder http://10.0.1.1 im Browser öffnen");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("4. Ihr Heim-WLAN auswählen und Passwort eingeben");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("5. Warten - MyStation verbindet sich und startet automatisch neu");
        y += lineHeight;

        u8g2.setFont(u8g2_font_helvB10_tf);
        u8g2.setCursor(margin, 460);
        u8g2.print("Hinweis: Nur 2,4-GHz-WLAN wird unterstützt. 5-GHz-Netzwerke werden nicht angezeigt.");

        // === QR CODES ON RIGHT SIDE ===
        const int16_t qrX = 540; // X position for QR codes (right side)
        const uint8_t qrVersion = 3; // Version 3 (29x29 modules)
        const uint8_t qrScale = 4; // 4 pixels per module
        int16_t qrSize = QRCodeHelper::getQRCodeSize(qrVersion, qrScale);

        // QR Code 1: WiFi Connection
        int16_t qr1Y = 80;
        QRCodeHelper::drawQRCode(qrX, qr1Y, wifiQR, qrScale, qrVersion);
        QRCodeHelper::drawQRLabel(qrX, qr1Y, qrSize, "1. WLAN verbinden", 15);

        // QR Code 2: Portal URL
        int16_t qr2Y = qr1Y + qrSize + 60; // Space between QR codes
        QRCodeHelper::drawQRCode(qrX, qr2Y, urlQR, qrScale, qrVersion);
        QRCodeHelper::drawQRLabel(qrX, qr2Y, qrSize, "2. Seite öffnen", 15);
    } while (display.nextPage());

    ESP_LOGI(TAG, "Phase 1 WiFi setup instructions displayed with QR codes");
}

void DisplayManager::displayPhase2AppSetup() {
    ESP_LOGI(TAG, "=== DISPLAYING PHASE 2: APP SETUP (GERMAN) ===");

    // Get dynamic SSID with hardware ID
    String apSSID = Util::getUniqueSSID("MyStation");

    ConfigManager::getInstance().loadFromNVS(false);
    RTCConfigData& config = ConfigManager::getConfig();
    String deviceIP = config.ipAddress;
    // Prepare QR code data - device configuration URL
    String configURL = "http://" + deviceIP;
    ESP_LOGI(TAG, "Config URL: %s", configURL.c_str());

    // Start display update
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Set up fonts
        u8g2.setFont(u8g2_font_helvB18_tf); // Bold 18pt for title
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        int16_t y = 40; // Start position from top
        const int16_t lineHeight = 35; // Spacing between lines
        const int16_t margin = 20; // Left margin

        // Draw title
        u8g2.setCursor(margin, y);
        u8g2.print("EINRICHTUNG 2/2 : MyStation konfigurieren");

        // Draw underline for title
        display.drawFastHLine(margin, y + 5, screenWidth - (2 * margin), GxEPD_BLACK);

        y += lineHeight + 5;

        // Draw instruction lines in German
        u8g2.setFont(u8g2_font_helvB10_tf);

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("1. Smartphone/PC mit Ihrem Heim-WLAN verbinden");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("(nicht mit \"" + apSSID + "\")");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("2. QR-Code scannen oder URL im Browser öffnen");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("3. Haltestelle, Anzeigemodus und Intervalle einstellen");
        y += lineHeight;

        y += 10;
        u8g2.setCursor(margin, y);
        u8g2.print("4. \"Speichern\" drücken - MyStation startet automatisch neu");
        y += lineHeight;

        // === QR CODE ON RIGHT SIDE ===
        const int16_t qrX = 540; // X position for QR code (right side)
        const uint8_t qrVersion = 3; // Version 3 (29x29 modules)
        const uint8_t qrScale = 4; // 4 pixels per module
        int16_t qrSize = QRCodeHelper::getQRCodeSize(qrVersion, qrScale);

        // QR Code: Configuration URL
        int16_t qrY = 120; // Centered vertically
        QRCodeHelper::drawQRCode(qrX, qrY, configURL, qrScale, qrVersion);
        QRCodeHelper::drawQRLabel(qrX, qrY, qrSize, configURL, 15);
        QRCodeHelper::drawQRLabel(qrX, qrY, qrSize, "http://mystation.local", 30);
    } while (display.nextPage());

    ESP_LOGI(TAG, "Phase 2 app setup instructions displayed with QR code");
}

// ===== APPLICATION INFO MODE =====

void DisplayManager::displayApplicationInfo(float batteryVoltage, int batteryPercent) {
    ESP_LOGI(TAG, "=== DISPLAYING APPLICATION INFO ===");

    RTCConfigData& cfg = ConfigManager::getConfig();

    // Determine board name string
#if defined(BOARD_S3_E1001)
    const char* boardName = "PCB E1001 (ESP32-S3)";
#elif defined(BOARD_S3_EE04)
    const char* boardName = "PCB EE04 (ESP32-S3)";
#elif defined(BOARD_C3_SUPERMINI)
    const char* boardName = "ESP32-C3 SuperMini";
#elif defined(BOARD_C5_XIAO)
    const char* boardName = "Seeed XIAO ESP32-C5";
#else
    const char* boardName = "Unknown Board";
#endif

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        const int16_t margin = 20;
        const int16_t col2 = screenWidth / 2 + margin; // second column x
        const int16_t lhSmall = 22; // line height for small font
        const int16_t lhBig = 30; // line height for section headers

        // ── Title bar ──────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB18_tf);
        int16_t y = 38;
        u8g2.setCursor(margin, y);
        u8g2.print("MyStation - Device Info");
        display.drawFastHLine(margin, y + 6, screenWidth - 2 * margin, GxEPD_BLACK);

        // ── LEFT COLUMN ────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        y += lhBig + 8;
        u8g2.setCursor(margin, y);
        u8g2.print("Firmware");
        display.drawFastHLine(margin, y + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Version : %s", FIRMWARE_VERSION);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        uint32_t chipId = (uint32_t)ESP.getEfuseMac() & 0xFFFFFF;
        u8g2.printf("Board   : %s [%06X]", boardName, chipId);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        {
#if BUILD_TIME != 0
            char buf[32];
            time_t bt_t = BUILD_TIME;
            struct tm* bt_tm = gmtime(&bt_t);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", bt_tm);
            u8g2.printf("Built   : %s", buf);
#else
            u8g2.print("Built   : debug build");
#endif
        }

        // Battery ──────────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        y += lhBig;
        u8g2.setCursor(margin, y);
        u8g2.print("Battery");
        display.drawFastHLine(margin, y + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        y += lhSmall;
        u8g2.setCursor(margin, y);
#if SHOW_BATTERY_STATUS
        if (batteryVoltage > 0.0f) {
            u8g2.printf("Voltage : %.2f V", batteryVoltage);
            y += lhSmall;
            u8g2.setCursor(margin, y);
            u8g2.printf("Level   : %d%%", batteryPercent);
        } else {
            u8g2.print("Not available");
        }
#else
        u8g2.print("Not supported");
#endif

        // ── Weather ────────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        y += lhBig;
        u8g2.setCursor(margin, y);
        u8g2.print("Weather");
        display.drawFastHLine(margin, y + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("City    : %s", cfg.cityName);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Lat/Lon : %.5f / %.5f", cfg.latitude, cfg.longitude);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Interval: %d hour(s)", cfg.weatherInterval);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        {
            const char* modelName = "Auto";
            if (strcmp(cfg.weatherModel, "icon_seamless") == 0) modelName = "DWD ICON (Germany)";
            else if (strcmp(cfg.weatherModel, "ecmwf_ifs025") == 0) modelName = "ECMWF (Europe)";
            else if (strcmp(cfg.weatherModel, "meteofrance_seamless") == 0) modelName = "Meteo-France (France)";
            else if (strcmp(cfg.weatherModel, "meteoswiss_icon_seamless") == 0) modelName = "MeteoSwiss (Switzerland)";
            else if (strcmp(cfg.weatherModel, "italia_meteo_arpae_icon_2i") == 0) modelName = "ItaliaMeteo (Italy)";
            u8g2.printf("Model   : %s", modelName);
        }

        // ── Transport ──────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        y += lhBig;
        u8g2.setCursor(margin, y);
        u8g2.print("Transport");
        display.drawFastHLine(margin, y + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Stop    : %s", cfg.selectedStopName);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Active  : %s - %s", cfg.transportActiveStart, cfg.transportActiveEnd);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        u8g2.printf("Interval: %d min", cfg.transportInterval);
        y += lhSmall;
        u8g2.setCursor(margin, y);
        // Build active vehicle filter string from bitmask
        {
            char filters[64] = "";
            if (cfg.filterFlags & FILTER_R) strncat(filters, "R ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_S) strncat(filters, "S ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_U) strncat(filters, "U ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_TRAM) strncat(filters, "Tram ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_BUS) strncat(filters, "Bus ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_HIGHFLOOR) strncat(filters, "HFloor ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_FERRY) strncat(filters, "Ferry ", sizeof(filters) - strlen(filters) - 1);
            if (cfg.filterFlags & FILTER_CALLBUS) strncat(filters, "CallBus ", sizeof(filters) - strlen(filters) - 1);
            if (strlen(filters) == 0) strncpy(filters, "None", sizeof(filters) - 1);
            // Trim trailing space
            int flen = strlen(filters);
            if (flen > 0 && filters[flen - 1] == ' ') filters[flen - 1] = '\0';
            u8g2.printf("Filters : %s", filters);
        }

        // ── RIGHT COLUMN ───────────────────────────────────────────────────
        int16_t ry = 38 + lhBig + 8; // Reset to same start as left col

        // Vertical divider
        display.drawFastVLine(screenWidth / 2, 44, screenHeight - 20 - 44, GxEPD_BLACK);

        // Network ──────────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        u8g2.setCursor(col2, ry);
        u8g2.print("Network");
        display.drawFastHLine(col2, ry + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("SSID    : %s", cfg.ssid);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("IP      : %s", cfg.ipAddress);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        {
            int rssi = CommonFooter::getCachedRSSI();
            const char* quality = "Very weak";
            if (rssi >= -50) quality = "Excellent";
            else if (rssi >= -60) quality = "Good";
            else if (rssi >= -70) quality = "Fair";
            else if (rssi >= -80) quality = "Weak";
            u8g2.printf("Signal  : %s (%d dBm)", quality, rssi);
        }

        // Display mode ────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        ry += lhBig;
        u8g2.setCursor(col2, ry);
        u8g2.print("Display");
        display.drawFastHLine(col2, ry + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        const char* modeName = "Unknown";
        switch (cfg.displayMode) {
        case DISPLAY_MODE_HALF_AND_HALF: modeName = "Half & Half";
            break;
        case DISPLAY_MODE_WEATHER_ONLY: modeName = "Weather Only";
            break;
        case DISPLAY_MODE_TRANSPORT_ONLY: modeName = "Transport Only";
            break;
        case DISPLAY_MODE_APPLICATION_INFO: modeName = "Application Info";
            break;
        }
        u8g2.printf("Mode    : %s", modeName);
        ry += lhSmall; // Spacer to align Sleep section with Weather on left

        // Sleep schedule ──────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        ry += lhBig;
        u8g2.setCursor(col2, ry);
        u8g2.print("Sleep");
        display.drawFastHLine(col2, ry + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("Window  : %s - %s", cfg.sleepStart, cfg.sleepEnd);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("Weekend : %s", cfg.weekendMode ? "Enabled" : "Disabled");
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("WE Sleep: %s - %s", cfg.weekendSleepStart, cfg.weekendSleepEnd);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("WE Trans: %s - %s", cfg.weekendTransportStart, cfg.weekendTransportEnd);

        // OTA ─────────────────────────────────────────────────────────────
        u8g2.setFont(u8g2_font_helvB12_tf);
        ry += lhBig;
        u8g2.setCursor(col2, ry);
        u8g2.print("OTA Update");
        display.drawFastHLine(col2, ry + 3, screenWidth / 2 - margin * 2, GxEPD_BLACK);

        u8g2.setFont(u8g2_font_helvB10_tf);
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("Enabled : %s", cfg.otaEnabled ? "Yes" : "No");
        ry += lhSmall;
        u8g2.setCursor(col2, ry);
        u8g2.printf("Time    : %s", cfg.otaCheckTime);

        // ── Footer ─────────────────────────────────────────────────────────
        display.drawFastHLine(margin, screenHeight - 20, screenWidth - 2 * margin, GxEPD_BLACK);
        u8g2.setFont(u8g2_font_helvB10_tf);
        u8g2.setCursor(margin, screenHeight - 6);
        {
            time_t now;
            time(&now);
            struct tm* t = localtime(&now);
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
            u8g2.printf("Generated: %s", buf);
        }
    } while (display.nextPage());

    ESP_LOGI(TAG, "Application info displayed");
}

// ===== ERROR DISPLAY METHODS =====

/**
 * @brief Display an error icon centered on the screen
 *
 * @param iconName The icon to display (from icon_name_t enum)
 * @param iconSize The size of the icon in pixels (16, 24, 32, 48, or 64)
 * @param message Optional error message to display below the icon
 */
void DisplayManager::displayCenteredErrorIcon(icon_name_t iconName, uint8_t iconSize,
                                              const char* message = nullptr) {
    // Calculate icon position (centered)
    int16_t iconX = halfWidth - (iconSize / 2);
    int16_t iconY = halfHeight - (iconSize / 2);

    ESP_LOGI(TAG, "Displaying error icon at center (%d, %d) with size %d", iconX, iconY, iconSize);

    // Get bitmap data
    const unsigned char* bitmap = getBitmap(iconName, iconSize);
    if (!bitmap) {
        ESP_LOGE(TAG, "Failed to get bitmap for icon %d at size %d", iconName, iconSize);
        return;
    }

    // Draw the icon centered
    display.drawInvertedBitmap(iconX, iconY, bitmap, iconSize, iconSize, GxEPD_BLACK);

    // Draw optional error message below icon
    if (message) {
        u8g2.setFont(u8g2_font_helvB10_tf); // 10pt bold font
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        // Calculate text wrapping
        int16_t maxWidth = screenWidth - 40; // 20px margin on each side
        int16_t lineHeight = 20; // Line spacing
        int16_t startY = iconY + iconSize + 30; // Start 30px below icon

        // Split message into lines that fit within maxWidth
        String msg = String(message);
        int16_t currentY = startY;
        int start = 0;
        while (start < msg.length()) {
            int end = msg.length();
            String line = msg.substring(start, end);

            // Find the longest substring that fits
            while (u8g2.getUTF8Width(line.c_str()) > maxWidth && end > start) {
                // Try to break at space
                int lastSpace = line.lastIndexOf(' ');
                if (lastSpace > 0) {
                    end = start + lastSpace;
                } else {
                    end--;
                }
                line = msg.substring(start, end);
            }

            // Draw the line centered
            int16_t textWidth = u8g2.getUTF8Width(line.c_str());
            int16_t textX = halfWidth - (textWidth / 2);
            u8g2.setCursor(textX, currentY);
            u8g2.print(line);

            // Move to next line
            start = end;
            if (start < msg.length() && msg.charAt(start) == ' ') {
                start++; // Skip the space
            }
            currentY += lineHeight;
        }
    }
}

void DisplayManager::displayErrorIfWifiConnectionError() {
    ESP_LOGW(TAG, "WiFi not connected - displaying error");

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Template: Change icon, size, and message here
        // Available icons: wifi_off, wifi_x, wifi_1_bar, wifi_2_bar, wifi_3_bar
        // Available sizes: 16, 24, 32, 48, 64
        displayCenteredErrorIcon(
            wifi_off, // Icon name
            64, // Icon size
            "Bitte überprüfen Sie Ihren WLAN-Router oder führen Sie einen Factory-Reset durch, um einen neuen Router zu verbinden."
        );
    } while (display.nextPage());

    ESP_LOGI(TAG, "WiFi error displayed");
}

void DisplayManager::displayErrorIfBatteryLow() {
    ESP_LOGW(TAG, "Battery low - displaying error");

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        // Template: Change icon, size, and message here
        // Available battery icons: Battery_1, Battery_2, Battery_3, Battery_4, Battery_5, battery_alert_0deg
        // Available sizes: 16, 24, 32, 48, 64
        displayCenteredErrorIcon(
            battery_alert_0deg, // Icon name
            64, // Icon size
            "Bitte laden Sie den Akku" // Error message (German: "Battery low")
        );
    } while (display.nextPage());

    ESP_LOGI(TAG, "Battery low error displayed");
}

// ===== OTA UPDATE STATUS =====

void DisplayManager::displayOTAProgress(const char* currentVersion, const char* targetVersion) {
    ESP_LOGI(TAG, "Displaying OTA progress: %s -> %s", currentVersion, targetVersion);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        const int16_t margin = 20;
        int16_t y = 200;

        u8g2.setFont(u8g2_font_helvB18_tf);
        u8g2.setCursor(margin, y);
        u8g2.print("Firmware-Update");

        y += 40;
        u8g2.setFont(u8g2_font_helvB10_tf);
        u8g2.setCursor(margin, y);
        u8g2.printf("%s  ->  %s", currentVersion, targetVersion);

        // Detect breaking change (major version bump)
        int curMajor = 0, tgtMajor = 0;
        sscanf(currentVersion[0] == 'v' ? currentVersion + 1 : currentVersion, "%d", &curMajor);
        sscanf(targetVersion[0] == 'v' ? targetVersion + 1 : targetVersion, "%d", &tgtMajor);
        bool breakingChange = (tgtMajor > curMajor);

        y += 40;
        u8g2.setFont(u8g2_font_helvB10_tf);
        u8g2.setCursor(margin, y);
        u8g2.print("Bitte nicht ausschalten!");

        if (breakingChange) {
            y += 30;
            u8g2.setCursor(margin, y);
            u8g2.print("Wichtig: Nach dem Update wird das Geraet");
            y += 18;
            u8g2.setCursor(margin, y);
            u8g2.print("zurueckgesetzt. Bitte neu konfigurieren.");
        }
    } while (display.nextPage());
}

void DisplayManager::displayOTAUpToDate(const char* currentVersion) {
    ESP_LOGI(TAG, "Displaying OTA up-to-date: %s", currentVersion);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);

        const int16_t margin = 20;
        int16_t y = 200;

        u8g2.setFont(u8g2_font_helvB18_tf);
        u8g2.setCursor(margin, y);
        u8g2.print("Firmware ist aktuell");

        y += 40;
        u8g2.setFont(u8g2_font_helvB10_tf);
        u8g2.setCursor(margin, y);
        u8g2.printf("Version: %s", currentVersion);
    } while (display.nextPage());
}
