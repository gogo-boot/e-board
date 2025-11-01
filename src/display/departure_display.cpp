#include "display/departure_display.h"
#include "display/text_utils.h"
#include "util/util.h"
#include "util/time_manager.h"
#include "display/display_shared.h"
#include "display/common_footer.h"
#include <esp_log.h>
#include <vector>
#include <icons.h>

static const char* TAG = "DEPARTURE_DISPLAY";

void DepartureDisplay::drawHalfScreenDepartureSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                                      int16_t h) {
    auto* display = DisplayShared::getDisplay();
    auto* u8g2 = DisplayShared::getU8G2();
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "Display not initialized! Call DisplayShared::init() first.");
        return;
    }
    ESP_LOGI(TAG, "Drawing departure section at (%d, %d) with size %dx%d", x, y, w, h);
    int16_t currentY = y; // Start from actual top
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    // Station name with TRUE 15px margin from top
    TextUtils::setFont14px_margin17px(); // Medium font for station name
    RTCConfigData& config = ConfigManager::getConfig();
    String stopName = getStopName(config);

    stopName = Util::Util::shortenStationName(stopName);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    // Print station name at top margin
    TextUtils::printTextAtTopMargin(leftMargin, currentY, fittedStopName);

    currentY += 17; // Space for station name
    currentY += 10; // Space after station name

    drawHalfScreenDepartures(departures, leftMargin, rightMargin, currentY, h - currentY);
}

void DepartureDisplay::drawFullScreenDepartures(const DepartureData& departures, int16_t x,
                                                int16_t y, int16_t w, int16_t h) {
    return;
}

void DepartureDisplay::drawHalfScreenDepartures(const DepartureData& departures, int16_t leftMargin,
                                                int16_t rightMargin, int16_t currentY, int16_t h) {
    // Half screen mode: Separate by direction flag
    ESP_LOGI(TAG, "Drawing departures separated by direction flag");

    std::vector<const DepartureInfo*> direction1Departures;
    std::vector<const DepartureInfo*> direction2Departures;
    getSeparatedDepatureDirection(departures, direction1Departures, direction2Departures);

    ESP_LOGI(TAG, "Found %d departures for direction 1, %d for direction 2",
             direction1Departures.size(), direction2Departures.size());

    // Draw separator line between directions
    int16_t halfHeightY = currentY + h / 2;
    ESP_LOGI(TAG, "Drawing departure direction separator line at Y=%d", halfHeightY);

    int8_t padding = 9; // Padding above and below the line
    auto* display = DisplayShared::getDisplay();
    display->drawLine(leftMargin, halfHeightY + padding, rightMargin, halfHeightY + padding, GxEPD_BLACK);

    constexpr int maxPerDirection = 5;

    drawDepartureList(direction1Departures, leftMargin, currentY, rightMargin - leftMargin, h - currentY,
                      true, maxPerDirection);

    currentY = halfHeightY + padding; // Reset currentY to halfHeightY for direction 2
    drawDepartureList(direction2Departures, leftMargin, currentY, rightMargin - leftMargin, h - currentY,
                      false, maxPerDirection);
}

void DepartureDisplay::drawDepartureList(std::vector<const DepartureInfo*> departure, int16_t x, int16_t y, int16_t w,
                                         int16_t h, bool printLabel, int maxPerDirection) {
    if (printLabel) {
        // Column headers with TRUE 12px margin from current position
        TextUtils::setFont10px_margin12px(); // Small font for column headers
        TextUtils::printTextAtTopMargin(x, y, "Soll    Ist      Linie     Ziel");

        y += 12;
        y += 5;

        // Underline
        auto* display = DisplayShared::getDisplay();
        display->drawLine(x, y, x + w, y, GxEPD_BLACK);
    }

    // Direction 1 departures
    for (int i = 0; i < min(maxPerDirection, (int)departure.size()); i++) {
        const auto& dep = *departure[i];
        drawSingleDeparture(dep, x, w, y);
        y += 42;

        if (y > DisplayShared::getScreenHeight()) {
            ESP_LOGW(TAG, "Reached end of section height while drawing departures");
            break; // Stop if we exceed the section height
        }
    }
}

void DepartureDisplay::getSeparatedDepatureDirection(const DepartureData& departures,
                                                     std::vector<const DepartureInfo*>& direction1Departures,
                                                     std::vector<const DepartureInfo*>& direction2Departures) {
    for (int i = 0; i < departures.departureCount; i++) {
        const auto& dep = departures.departures[i];
        if (dep.directionFlag == "1" || dep.directionFlag.toInt() == 1) {
            direction1Departures.push_back(&dep);
        } else if (dep.directionFlag == "2" || dep.directionFlag.toInt() == 2) {
            direction2Departures.push_back(&dep);
        }
    }
}

void DepartureDisplay::drawFullScreenDepartureSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                                      int16_t h) {
    // Half screen mode: Separate by direction flag
    ESP_LOGI(TAG, "drawFullScreenDepartureSection at (%d, %d) with size %dx%d", x, y, w, h);
    int16_t currentY = y; // Start from actual top
    const int16_t padding = 10;
    const int16_t leftMargin = x + 10;
    const int16_t rightMargin = x + w - 10;

    // Station name with TRUE 15px margin from top
    TextUtils::setFont14px_margin17px(); // Medium font for station name
    RTCConfigData& config = ConfigManager::getConfig();
    const String stopName = getStopName(config);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    // Print station name at top margin
    TextUtils::printTextAtTopMargin(leftMargin, currentY, fittedStopName);

    String dateTime = TimeManager::getGermanDateTimeString();
    int16_t dateTimeWidth = TextUtils::getTextWidth(dateTime);

    int16_t refreshIconWidth = 16; // Width of the refresh icon
    TextUtils::printTextAtTopMargin(rightMargin - dateTimeWidth - refreshIconWidth - 10, currentY, dateTime);

    auto* display = DisplayShared::getDisplay();
    display->drawInvertedBitmap(rightMargin - refreshIconWidth, currentY, getBitmap(refresh, 16), 16, 16, GxEPD_BLACK);

    currentY += 17; // Space for station name
    currentY += 25; // Space after station name

    std::vector<const DepartureInfo*> direction1Departures;
    std::vector<const DepartureInfo*> direction2Departures;
    getSeparatedDepatureDirection(departures, direction1Departures, direction2Departures);

    ESP_LOGI(TAG, "Found %d departures for direction 1, %d for direction 2",
             direction1Departures.size(), direction2Departures.size());

    // Draw first 4 departures from direction 1
    constexpr int maxPerDirection = 10;

    const int16_t halfWidth = DisplayShared::getScreenWidth() / 2 - 1;
    drawDepartureList(direction1Departures, x + padding, currentY, halfWidth - padding, h - currentY, true,
                      maxPerDirection);
    drawDepartureList(direction2Departures, halfWidth + padding, currentY, halfWidth - padding, h - currentY,
                      true, maxPerDirection);
}

void DepartureDisplay::drawSingleDeparture(const DepartureInfo& dep, int16_t x, int16_t width,
                                           int16_t currentY) {
    auto* u8g2 = DisplayShared::getU8G2();
    if (!u8g2) return;

    // Log the departure position and size
    ESP_LOGI(TAG, "Drawing single departure at Y=%d", currentY);

    currentY += 3;
    // Half screen format with proper text positioning
    TextUtils::setFont10px_margin12px(); // Small font for departure entries

    // Calculate available space
    int totalWidth = width - x;

    // Check if times are different for highlighting
    bool timesAreDifferent = (dep.rtTime.length() > 0 && dep.rtTime != dep.time);

    // Clean up destination (remove "Frankfurt (Main)" prefix)
    RTCConfigData& config = ConfigManager::getConfig();
    const String stopName = getStopName(config);
    String dest = Util::shortenDestination(stopName, dep.direction);

    // Prepare times
    String sollTime = dep.time.substring(0, 5);
    String istTime = "";

    if (!timesAreDifferent) {
        istTime = "  +00"; // Use "00" to indicate on-time
    } else if (dep.rtTime.length() > 0) {
        // Calculate minute difference between scheduled and real-time
        int scheduledMinutes = dep.time.substring(3, 5).toInt() + dep.time.substring(0, 2).toInt() * 60;
        int realTimeMinutes = dep.rtTime.substring(3, 5).toInt() + dep.rtTime.substring(0, 2).toInt() * 60;
        int diffMinutes = realTimeMinutes - scheduledMinutes;

        if (diffMinutes > 0) {
            istTime = "  +" + String(diffMinutes);
        }
    }

    // get max width for each column
    int8_t timeWidth = TextUtils::getTextWidth("88:88");
    int8_t lineWidth = TextUtils::getTextWidth("M888");
    int8_t padding = 10;

    int16_t currentX = x;

    // Print times with strikethrough if cancelled
    if (dep.cancelled) {
        TextUtils::printStrikethroughTextAtTopMargin(currentX, currentY, sollTime.c_str());
    } else {
        TextUtils::printTextAtTopMargin(currentX, currentY, sollTime.c_str());
    }

    currentX += padding + timeWidth;

    if (dep.cancelled) {
        TextUtils::printStrikethroughTextAtTopMargin(currentX, currentY, istTime.c_str());
    } else {
        TextUtils::printTextAtTopMargin(currentX, currentY, istTime.c_str());
    }

    currentX += padding + timeWidth;
    TextUtils::printTextAtTopMargin(currentX, currentY, dep.line.c_str());
    currentX += padding + lineWidth;
    TextUtils::printTextAtTopMargin(currentX, currentY, dest.c_str());

    // Draw track info right-aligned
    int8_t trackWidth = TextUtils::getTextWidth(dep.track.c_str());
    currentX = x + width - trackWidth - 15; // 10px padding from right
    TextUtils::printTextAtTopMargin(currentX, currentY, dep.track.c_str());

    currentY += 17; // per entry height
    currentY += 3; // Add spacing after departure entry

    // Check if we have disruption information to display
    if (dep.cancelled) {
        int8_t indent = 10;
        TextUtils::printTextAtTopMargin(x + indent, currentY, "Fällt aus");
    } else if (dep.lead.length() > 0 || dep.text.length() > 0) {
        // Use the lead text if available, otherwise use text
        String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;

        int8_t indent = 10;
        // Fit disruption text to available width
        int disruptionMaxWidth = width - indent;
        String fittedDisruption = TextUtils::shortenTextToFit(disruptionInfo, disruptionMaxWidth);

        // Display disruption information with proper positioning
        TextUtils::printTextAtTopMargin(x + indent, currentY, fittedDisruption);
    }
}

void DepartureDisplay::drawDepartureFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_BATTERY);
}

String DepartureDisplay::getStopName(RTCConfigData& config) {
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
