#include "display/departure_display.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include <esp_log.h>
#include <vector>

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

    int16_t currentY = y; // Start from actual top
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    // Station name with TRUE 15px margin from top
    TextUtils::setFont12px_margin15px(); // Medium font for station name
    int16_t stationTextTop = currentY + 15; // 15px margin from top
    RTCConfigData &config = ConfigManager::getConfig();
    String stopName = getStopName(config);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    TextUtils::printTextAtTopMargin(leftMargin, stationTextTop, fittedStopName);
    currentY = stationTextTop + 12 + 13; // text height + spacing = 25px total

    // Column headers with TRUE 12px margin from current position
    TextUtils::setFont10px_margin12px(); // Small font for column headers
    int16_t headerTextTop = currentY + 12; // 12px margin
    if (isFullScreen) {
        TextUtils::printTextAtTopMargin(leftMargin, headerTextTop, "Soll Ist  Linie  Ziel                Gleis");
    } else {
        TextUtils::printTextAtTopMargin(leftMargin, headerTextTop, "Soll    Ist      Linie     Ziel");
    }
    currentY = headerTextTop + 10 + 8; // text height + spacing = 18px total

    // Underline
    display->drawLine(leftMargin, currentY - 5, rightMargin, currentY - 5, GxEPD_BLACK);
    currentY += 12; // Header underline spacing

    if (isFullScreen) {
        drawFullScreenDepartures(departures, leftMargin, rightMargin, currentY, y, h);
    } else {
        drawHalfScreenDepartures(departures, leftMargin, rightMargin, currentY, y, h);
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
                                              int16_t rightMargin, int16_t &currentY, int16_t y, int16_t h) {
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
    
    // Draw first 4 departures from direction 1
    int drawnCount = 0;
    int maxPerDirection = 4;
    
    // Direction 1 departures
    for (int i = 0; i < min(maxPerDirection, (int)direction1Departures.size()) && drawnCount < 8; i++) {
        const auto &dep = *direction1Departures[i];
        drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
        drawnCount++;
        
        if (currentY > y + h - 60) break; // Leave space for separator and direction 2
    }
    
    // Draw separator line between directions
    if (direction1Departures.size() > 0 && direction2Departures.size() > 0 && drawnCount < 8) {
        display->drawLine(leftMargin, currentY, rightMargin, currentY, GxEPD_BLACK);
        currentY += 15; // Space after separator line
    }
    
    // Direction 2 departures
    for (int i = 0; i < min(maxPerDirection, (int)direction2Departures.size()) && drawnCount < 8; i++) {
        const auto &dep = *direction2Departures[i];
        drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
        drawnCount++;
        
        if (currentY > y + h - 25) break; // Leave space for footer
    }
    
    ESP_LOGI(TAG, "Drew %d total departures", drawnCount);
}

void DepartureDisplay::drawSingleDeparture(const DepartureInfo &dep, int16_t leftMargin, int16_t rightMargin, 
                                         int16_t &currentY, bool isFullScreen) {
    if (!u8g2) return;

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
        // Half screen format with proper text positioning
        TextUtils::setFont10px_margin12px(); // Small font for departure entries
        
        // Calculate text top position with proper margin
        int16_t textTop = currentY + 12; // 12px margin from function name
        
        // Calculate available space
        int totalWidth = rightMargin - leftMargin;
        
        // Check if times are different for highlighting
        bool timesAreDifferent = (dep.rtTime.length() > 0 && dep.rtTime != dep.time);
        
        // Clean up line (remove "Bus" prefix)
        String line = dep.line;
        String lineLower = line;
        lineLower.toLowerCase();
        if (lineLower.startsWith("bus ")) {
            line = line.substring(4);
        }
        
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
        
        // Measure fixed elements: times and spaces
        int timesWidth = TextUtils::getTextWidth(sollTime + "  " + istTime + "  ");
        int remainingWidth = totalWidth - timesWidth;
        
        // Position text at proper baseline
        int16_t baseline = textTop + TextUtils::getFontAscent();
        
        // Print soll time
        u8g2->setCursor(leftMargin, baseline);
        u8g2->print(sollTime);
        u8g2->print(" ");
        
        // Calculate current cursor position for ist time highlighting
        int16_t istX = leftMargin + TextUtils::getTextWidth(sollTime + " ");
        
        if (timesAreDifferent) {
            // Highlight ist time with underline
            int16_t istTextWidth = TextUtils::getTextWidth(istTime);
            
            // Print the delayed time normally first
            u8g2->setCursor(istX, baseline);
            u8g2->print(istTime);
            
            // Draw underline below the text baseline
            int16_t underlineY = baseline + 2;
            display->drawLine(istX, underlineY, istX + istTextWidth, underlineY, GxEPD_BLACK);
        } else {
            // Normal ist time display
            u8g2->print(istTime);
        }
        
        // Allocate remaining space: Line gets 1/3, Destination gets 2/3
        int lineMaxWidth = remainingWidth / 3;
        int destMaxWidth = (remainingWidth * 2) / 3;
        
        // Fit line and destination to available space
        String fittedLine = TextUtils::shortenTextToFit(line, lineMaxWidth);
        String fittedDest = TextUtils::shortenTextToFit(dest, destMaxWidth);
        
        u8g2->print("  ");
        u8g2->print(fittedLine);
        u8g2->print("  ");
        u8g2->print(fittedDest);
    }
    
    // Always add consistent spacing for disruption area
    currentY += 20; // Main departure line gets 20px
    
    // Check if we have disruption information to display
    if (dep.lead.length() > 0 || dep.text.length() > 0) {
        // Use the lead text if available, otherwise use text
        String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;
        
        // Fit disruption text to available width with some indent
        int disruptionMaxWidth = rightMargin - leftMargin - 20; // 20px indent
        String fittedDisruption = TextUtils::shortenTextToFit(disruptionInfo, disruptionMaxWidth);
        
        // Display disruption information with proper positioning
        TextUtils::setFont10px_margin12px(); // Small font for disruption info
        int16_t disruptionTextTop = currentY + 12; // 12px margin
        TextUtils::printTextAtTopMargin(leftMargin + 20, disruptionTextTop, "⚠ " + fittedDisruption);
    }
    
    // Add consistent spacing after disruption area (whether used or not)
    currentY += 17; // Disruption space gets 17px (total 37px per entry)
}

void DepartureDisplay::drawDepartureFooter(int16_t x, int16_t y) {
    if (!u8g2) {
        ESP_LOGE(TAG, "DepartureDisplay not initialized! Call init() first.");
        return;
    }

    TextUtils::setFont10px_margin12px(); // Small font for footer

    // Ensure footer is positioned properly within bounds
    int16_t footerY = min(y, (int16_t)(screenHeight - 14)); // Ensure space from bottom
    int16_t footerX = x + 10;
    ESP_LOGI(TAG, "Departure footer position: (%d, %d)", footerX, footerY);

    // Position footer text with proper baseline calculation
    int16_t baseline = footerY + TextUtils::getFontAscent();
    u8g2->setCursor(footerX, baseline);
    u8g2->print("Aktualisiert: ");

    // Check if time is properly set
    if (TimeManager::isTimeSet()) {
        // Get current German time using TimeManager
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char timeStr[20];
            // German time format: "HH:MM DD.MM."
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.", &timeinfo);
            u8g2->print(timeStr);
        } else {
            u8g2->print("Zeit nicht verfügbar");
        }
    } else {
        // Time not synchronized
        u8g2->print("Zeit nicht synchronisiert");
    }
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
