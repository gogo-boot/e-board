#include "display/departure_display.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include <esp_log.h>
#include <vector>
#include <icons.h>

static const char *TAG = "DEPARTURE_DISPLAY";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *DepartureDisplay::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX *DepartureDisplay::u8g2 = nullptr;
int16_t DepartureDisplay::screenWidth = 0;
int16_t DepartureDisplay::screenHeight = 0;

void DepartureDisplay::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                           U8G2_FOR_ADAFRUIT_GFX &u8g2Ref, 
                           int16_t screenW, int16_t screenH) {
    display = &displayRef;
    u8g2 = &u8g2Ref;
    screenWidth = screenW;
    screenHeight = screenH;
    
    ESP_LOGI(TAG, "DepartureDisplay initialized with screen size %dx%d", screenW, screenH);
}

void DepartureDisplay::drawDepartureSection(const DepartureData &departures, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "DepartureDisplay not initialized! Call init() first.");
        return;
    }
    ESP_LOGI(TAG, "Drawing departure section at (%d, %d) with size %dx%d", x, y, w, h); 
    int16_t currentY = y; // Start from actual top
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    // Station name with TRUE 15px margin from top
    TextUtils::setFont14px_margin17px(); // Medium font for station name
    int16_t stationTextTop = currentY + 22; // 15px margin from top
    RTCConfigData &config = ConfigManager::getConfig();
    String stopName = getStopName(config);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    // Print station name at top margin
    TextUtils::printTextAtTopMargin(leftMargin, currentY, fittedStopName);

    currentY += 17; // Space for station name 
    currentY += 10; // Space after station name

    // Column headers with TRUE 12px margin from current position
    TextUtils::setFont10px_margin12px(); // Small font for column headers
    if (isFullScreen) {
        TextUtils::printTextAtTopMargin(leftMargin, currentY, "Soll Ist  Linie  Ziel                Gleis");
    } else {
        TextUtils::printTextAtTopMargin(leftMargin, currentY, "Soll    Ist      Linie     Ziel");
    }
    currentY += 12; 
    currentY += 5;

    // Underline
    display->drawLine(leftMargin, currentY, rightMargin, currentY, GxEPD_BLACK);
 
    // 421 px space left for departures
    if (isFullScreen) {
        drawFullScreenDepartures(departures, leftMargin, rightMargin, currentY, y, h);
    } else {
        drawHalfScreenDepartures(departures, leftMargin, rightMargin, currentY, h - currentY);
    }
}

void DepartureDisplay::drawFullScreenDepartures(const DepartureData &departures, int16_t leftMargin, 
                                              int16_t rightMargin, int16_t &currentY, int16_t y, int16_t h) {
    // Original full screen logic
    int maxDepartures = min(20, departures.departureCount);
    
    for (int i = 0; i < maxDepartures; i++) {
        const auto &dep = departures.departures[i];
        
        // TODO: Implement full screen departure drawing logic
        drawSingleDeparture(dep, leftMargin, rightMargin, currentY, true); // true = full screen
        
        if (currentY > y + h - 25) break; // Leave space for footer
    }
}

void DepartureDisplay::drawHalfScreenDepartures(const DepartureData &departures, int16_t leftMargin, 
                                              int16_t rightMargin, int16_t currentY, int16_t h) {
    // Half screen mode: Separate by direction flag
    ESP_LOGI(TAG, "Drawing departures separated by direction flag");
    
    // Separate departures by direction flag
    std::vector<const DepartureInfo*> direction1Departures;
    std::vector<const DepartureInfo*> direction2Departures;
    
    for (int i = 0; i < departures.departureCount; i++) {
        const auto &dep = departures.departures[i];
        if (dep.directionFlag == "1" || dep.directionFlag.toInt() == 1) {
            direction1Departures.push_back(&dep);
        } else if (dep.directionFlag == "2" || dep.directionFlag.toInt() == 2) {
            direction2Departures.push_back(&dep);
        }
    }
    
    ESP_LOGI(TAG, "Found %d departures for direction 1, %d for direction 2", 
             direction1Departures.size(), direction2Departures.size());
    
    // Draw separator line between directions
    int16_t halfHeightY = currentY + h / 2;
    //log the position of the separator line, and y 
    ESP_LOGI(TAG, "Drawing separator line at Y=%d", halfHeightY);
    display->drawLine(leftMargin, halfHeightY, rightMargin, halfHeightY, GxEPD_BLACK);

    // Draw first 4 departures from direction 1
    int drawnCount = 0;
    int maxPerDirection = 5;
    
    // Direction 1 departures
    for (int i = 0; i < min(maxPerDirection, (int)direction1Departures.size()); i++) {
        const auto &dep = *direction1Departures[i];
        drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
        currentY += 42;
        drawnCount++;
    }

    currentY = halfHeightY + 1; // Reset currentY to halfHeightY for direction 2
    // Direction 2 departures
    for (int i = 0; i < min(maxPerDirection, (int)direction2Departures.size()); i++) {
        const auto &dep = *direction2Departures[i];
        // log the currentY position and halfHeightY
        ESP_LOGI(TAG, "Drawing direction 2 departure at Y=%d (halfHeightY=%d)", currentY, halfHeightY);
        drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
        currentY += 42;
        drawnCount++;
    }
    
    ESP_LOGI(TAG, "Drew %d total departures", drawnCount);
}

void DepartureDisplay::drawSingleDeparture(const DepartureInfo &dep, int16_t leftMargin, int16_t rightMargin, 
                                         int16_t currentY, bool isFullScreen) {
    if (!u8g2) return;

    // Log the departure position and size
    ESP_LOGI(TAG, "Drawing single departure at Y=%d with full screen=%d", currentY, isFullScreen); 

    if (isFullScreen) {
        // Full screen format (existing logic)
        // TODO: Implement full screen single departure drawing
        TextUtils::setFont10px_margin12px();
        int16_t textTop = currentY + 12;
        int16_t baseline = textTop + TextUtils::getFontAscent();
        u8g2->setCursor(leftMargin, baseline);
        u8g2->print(dep.time.substring(0, 5) + " " + dep.line + " " + dep.direction);
        currentY += 37; // Total spacing per entry
    } else {
        currentY += 3;
        // Half screen format with proper text positioning
        TextUtils::setFont10px_margin12px(); // Small font for departure entries
        
        // Calculate available space
        int totalWidth = rightMargin - leftMargin;
        
        // Check if times are different for highlighting
        bool timesAreDifferent = (dep.rtTime.length() > 0 && dep.rtTime != dep.time);
        
        // Clean up destination (remove "Frankfurt (Main)" prefix)
        String dest = dep.direction;
        String destLower = dest;
        destLower.toLowerCase();
        if (destLower.startsWith("frankfurt (main) ")) {
            dest = dep.direction.substring(17);
        }
        
        // Prepare times
        String sollTime = dep.time.substring(0, 5);
        String istTime = dep.rtTime.length() > 0 ? dep.rtTime.substring(0, 5) : dep.time.substring(0, 5);

        if (!timesAreDifferent) {
            istTime = "  +00"; // Use "00" to indicate on-time
        }

        String singleDeparture = sollTime + " " + istTime + " " + dep.line + " " + dest + " " + dep.track;
        TextUtils::printTextAtTopMargin(leftMargin, currentY, singleDeparture);
        currentY += 17; // per entry height
        currentY += 3; // Add spacing after departure entry

        // Check if we have disruption information to display
        if (dep.lead.length() > 0 || dep.text.length() > 0) {
            // Use the lead text if available, otherwise use text
            String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;
            
            // Fit disruption text to available width with some indent
            int disruptionMaxWidth = rightMargin - leftMargin - 20; // 20px indent
            String fittedDisruption = TextUtils::shortenTextToFit(disruptionInfo, disruptionMaxWidth);
            
            // Display disruption information with proper positioning
            TextUtils::setFont10px_margin12px(); // Small font for disruption info
            TextUtils::printTextAtTopMargin(leftMargin + 20, currentY, fittedDisruption);
        }
    }
}

void DepartureDisplay::drawDepartureFooter(int16_t x, int16_t y, int16_t h) {
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "WeatherDisplay not initialized! Call init() first.");
        return;
    }
    TextUtils::setFont10px_margin12px(); // Small font for footer

    int16_t footerY = y + h - 14; // Correct: bottom of section
    int16_t footerX = x + 10;

    String footerText = "";
    if (TimeManager::isTimeSet()) {
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.", &timeinfo);
            footerText += String(timeStr);
        } else {
            footerText += "Zeit nicht verfügbar";
        }
    } else {
        footerText += "Zeit nicht synchronisiert";
    }
    TextUtils::printTextAtWithMargin(footerX, footerY, footerText);

    int16_t timeStrWidth = TextUtils::getTextWidth(footerText);
    display->drawInvertedBitmap( footerX + timeStrWidth + 5, y , getBitmap(refresh, 16), 16, 16, GxEPD_BLACK);
}

String DepartureDisplay::getStopName(RTCConfigData &config) {
    String stopName = config.selectedStopId;

    // Extract stop name from stopId format: "@O=StopName@"
    int startIndex = stopName.indexOf("@O=");
    if (startIndex != -1) {
        startIndex += 3; // Move past "@O="
        int endIndex = stopName.indexOf("@", startIndex);
        if (endIndex != -1) {
            stopName = stopName.substring(startIndex, endIndex);
            return stopName;
        }
    }
    return "";
}
