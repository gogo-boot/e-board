#include <Arduino.h>
#include "display/display_manager.h"

#include "config/config_manager.h"
#include "display/transport_display.h"
#include "display/weather_general_half.h"
#include "display/weather_general_full.h"
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

        y += lineHeight + 10; // Extra space after title

        // Draw instruction lines in German
        u8g2.setFont(u8g2_font_helvB10_tf); // Regular 10pt for content

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("1. Schalten Sie MyStation an");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print(
            "2. Mit Smartphone/PC mit dem MyStation-Netzwerk verbinden");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("1. QR-Code scannen");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("3. Seite erscheint nicht automatisch?");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("ggf. 2. QR-Code scannen");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("4. Wählen Sie Ihr WLAN aus");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("und geben Sie Ihre WLAN-Zugangsdaten ein");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("5. Warten Sie etwa 10 Sekunden");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("System prüft Internetverbindung und leitet nächsten Schritt ein");
        y += lineHeight;

        y += 20; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("MyStation braucht die Internetverbindung für Zeit, Wetter- und Verkehrsdaten");
        y += lineHeight;

        // === QR CODES ON RIGHT SIDE ===
        const int16_t qrX = 540; // X position for QR codes (right side)
        const uint8_t qrVersion = 3; // Version 3 (29x29 modules)
        const uint8_t qrScale = 4; // 4 pixels per module
        int16_t qrSize = QRCodeHelper::getQRCodeSize(qrVersion, qrScale);

        // QR Code 1: WiFi Connection
        int16_t qr1Y = 80;
        QRCodeHelper::drawQRCode(qrX, qr1Y, wifiQR, qrScale, qrVersion);
        QRCodeHelper::drawQRLabel(qrX, qr1Y, qrSize, "1. " + apSSID, 15);

        // QR Code 2: Portal URL
        int16_t qr2Y = qr1Y + qrSize + 60; // Space between QR codes
        QRCodeHelper::drawQRCode(qrX, qr2Y, urlQR, qrScale, qrVersion);
        QRCodeHelper::drawQRLabel(qrX, qr2Y, qrSize, "2. " + urlQR, 15);
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
        u8g2.print("EINRICHTUNG 2/2 : MyStation Anwendungskonfiguration");

        // Draw underline for title
        display.drawFastHLine(margin, y + 5, screenWidth - (2 * margin), GxEPD_BLACK);

        y += lineHeight + 10; // Extra space after title

        // Draw instruction lines in German
        u8g2.setFont(u8g2_font_helvB10_tf); // Regular 10pt for content

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("1. Verbinden Sie sich mit Ihrem WLAN");
        y += lineHeight;

        u8g2.setCursor(margin + 20, y);
        u8g2.print("(nicht mit dem " + apSSID + " WLAN)");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("2. QR-Code scannen oder angezeigte URL im Browser eingeben");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("3. Konfigurieren Sie MyStation im Webbrowser");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("4. Speichern Sie die Konfiguration und warten Sie etwa 10 Sekunden.");
        y += lineHeight;

        y += 10; // Extra spacing
        u8g2.setCursor(margin, y);
        u8g2.print("MyStation startet automatisch neu");
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
