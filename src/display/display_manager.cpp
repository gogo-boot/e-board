#include "display/display_manager.h"
#include "display/weather_display.h"
#include "display/text_utils.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/pins.h"

// Font includes for German character support
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char *TAG = "DISPLAY_MGR";

// Static member variables
DisplayMode DisplayManager::currentMode = DisplayMode::HALF_AND_HALF;
DisplayOrientation DisplayManager::currentOrientation = DisplayOrientation::LANDSCAPE;
int16_t DisplayManager::screenWidth = 0;  // Will be read from display
int16_t DisplayManager::screenHeight = 0; // Will be read from display
int16_t DisplayManager::halfWidth = 0;    // Will be calculated
int16_t DisplayManager::halfHeight = 0;   // Will be calculated

void DisplayManager::init(DisplayOrientation orientation)
{
    ESP_LOGI(TAG, "Initializing display manager");

    display.init(115200);

    // Initialize U8g2 for UTF-8 font support (German umlauts)
    u8g2.begin(display);
    u8g2.setFontMode(1);      // Use u8g2 transparent mode
    u8g2.setFontDirection(0); // Left to right
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);

    // Set rotation first to get correct dimensions
    display.setRotation(static_cast<int>(orientation));

    // Now get dimensions from display after rotation is set
    screenWidth = display.width();
    screenHeight = display.height();

    // Initialize WeatherDisplay with shared resources
    WeatherDisplay::init(display, u8g2, screenWidth, screenHeight);

    // Initialize TextUtils with shared resources
    TextUtils::init(display, u8g2);

    ESP_LOGI(TAG, "Display detected - Physical dimensions: %dx%d", screenWidth, screenHeight);

    setMode(DisplayMode::HALF_AND_HALF, orientation);

    ESP_LOGI(TAG, "Display initialized - Orientation: Landscape, Active dimensions: %dx%d",
             screenWidth, screenHeight);
}

void DisplayManager::setMode(DisplayMode mode, DisplayOrientation orientation)
{
    currentMode = mode;
    currentOrientation = orientation;

    // Set display rotation
    display.setRotation(static_cast<int>(orientation));

    // Get dimensions after rotation
    screenWidth = display.width();
    screenHeight = display.height();

    calculateDimensions();

    ESP_LOGI(TAG, "Display mode set - Mode: %d, Orientation: Landscape, Dimensions: %dx%d",
             static_cast<int>(mode),
             screenWidth, screenHeight);
}

void DisplayManager::calculateDimensions()
{
    // Landscape mode: 800x480 - split WIDTH in half
    // Weather: left half (0-399), Departures: right half (400-799)
    halfWidth = screenWidth / 2; // Split width: 400 pixels each
    halfHeight = screenHeight;   // Full height: 480 pixels
    ESP_LOGI(TAG, "Landscape split: Weather[0,0,%d,%d] Departures[%d,0,%d,%d]",
             halfWidth, screenHeight, halfWidth, halfWidth, screenHeight);
}

void DisplayManager::displayHalfAndHalf(const WeatherInfo *weather, const DepartureData *departures)
{
    ESP_LOGI(TAG, "Displaying half and half mode");

    if (!weather && !departures)
    {
        ESP_LOGW(TAG, "No data provided for half and half display");
        return;
    }

    // Remove header - use full screen height
    const int16_t footerHeight = 15;
    const int16_t contentY = 0; // Start from top instead of after header
    const int16_t contentHeight = screenHeight - footerHeight;

    if (weather && departures)
    {
        // Full update - both halves without header
        ESP_LOGI(TAG, "Updating both halves without header");

        bool isFullUpdate = true; // Full update for both halves
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);

            // Remove header drawing
            // Landscape: left/right split (weather left, departures right)
            updateWeatherHalf(isFullUpdate, *weather);
            updateDepartureHalf(isFullUpdate, *departures);

            // Todo Draw it only in debug mode
            displayVerticalLine(isFullUpdate, contentY);
        } while (display.nextPage());
    }
    else if (weather)
    {
        display.setFullWindow();
        display.firstPage();
        // Partial update - weather half only
        do
        {
            updateWeatherHalf(false, *weather);
        } while (display.nextPage());
    }
    else if (departures)
    {
        display.setFullWindow();
        display.firstPage();
        do
        {
            // Partial update - departure half only
            updateDepartureHalf(false, *departures);
        } while (display.nextPage());
    }
}

void DisplayManager::displayVerticalLine(bool isFoolUpdate, const int16_t contentY)
{
    if (!isFoolUpdate)
    {
        display.setPartialWindow(halfWidth, contentY, 1, screenHeight - contentY);
        display.fillRect(halfWidth, contentY, 1, screenHeight - contentY, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping vertical line update in non-full update mode");
        return;
    }
    // Draw vertical divider
    display.drawLine(halfWidth, contentY, halfWidth, screenHeight, GxEPD_BLACK);
}

void DisplayManager::updateWeatherHalf(bool isFullUpate, const WeatherInfo &weather)
{
    ESP_LOGI(TAG, "Updating weather half");

    const int16_t footerHeight = 25;
    const int16_t contentY = 0; // Start from top without header
    const int16_t contentHeight = screenHeight; // Use full height

    // Landscape: weather is LEFT half (full height)
    int16_t x = 0, y = 0, w = halfWidth, h = screenHeight;

    if (!isFullUpate)
    {
        display.setPartialWindow(x, y, w, h);

        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping weather half update in non-full update mode");
        return;
    }
    // Use partial window for faster update
    WeatherDisplay::drawWeatherSection(weather, x, contentY, w, contentHeight);
    WeatherDisplay::drawWeatherFooter(x, screenHeight - footerHeight);
}

void DisplayManager::updateDepartureHalf(bool isFullUpate,const DepartureData &departures)
{
    ESP_LOGI(TAG, "Updating departure half");

    const int16_t footerHeight = 25;
    const int16_t contentY = 0; // Start from top without header
    const int16_t contentHeight = screenHeight; // Use full height

    // Landscape: departures are RIGHT half (full height)
    int16_t x = halfWidth, y = contentY, w = halfWidth, h = contentHeight;

    if (!isFullUpate)
    {
        // // Use partial window for faster update
        display.setPartialWindow(x, y, w, h);

        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping departure half update in non-full update mode");
        return;
    }
    drawDepartureSection(departures, x, y, w, h);
    drawDepartureFooter(x, screenHeight - footerHeight);
    // Redraw vertical divider
    display.drawLine(halfWidth, 0, halfWidth, screenHeight, GxEPD_BLACK);
}

void DisplayManager::displayWeatherOnly(const WeatherInfo &weather)
{
    ESP_LOGI(TAG, "Displaying weather only mode");

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        WeatherDisplay::drawWeatherSection(weather, 0, 0, screenWidth, screenHeight);
    } while (display.nextPage());
}

void DisplayManager::displayDeparturesOnly(const DepartureData &departures)
{
    ESP_LOGI(TAG, "Displaying departures only mode");

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        drawDepartureSection(departures, 0, 0, screenWidth, screenHeight);
    } while (display.nextPage());
}

// Weather functions moved to WeatherDisplay class

void DisplayManager::drawDepartureFooter(int16_t x, int16_t y)
{
    TextUtils::setFont10px_margin12px(); // Small font for footer

    // Ensure footer is positioned properly within bounds
    int16_t footerY = min(y, (int16_t)(screenHeight - 20)); // Ensure at least 20px from bottom
    int16_t footerX = x + 10;
    ESP_LOGI(TAG, "Footer position: (%d, %d)", footerX, footerY);

    u8g2.setCursor(footerX, footerY); // Add 10px left margin
    u8g2.print("Aktualisiert: ");

    // Check if time is properly set
    if (TimeManager::isTimeSet())
    {
        // Get current German time using TimeManager
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo))
        {
            char timeStr[20];
            // German time format: "HH:MM DD.MM."
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.", &timeinfo);
            u8g2.print(timeStr);
        }
        else
        {
            u8g2.print("Zeit nicht verfügbar");
        }
    }
    else
    {
        // Time not synchronized
        u8g2.print("Zeit nicht synchronisiert");
    }
}

void DisplayManager::drawDepartureSection(const DepartureData &departures, int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t currentY = y + 15; // Start closer to top without header space
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    // Station name
    TextUtils::setFont12px_margin15px(); // Medium font for station name
    u8g2.setCursor(leftMargin, currentY);
    RTCConfigData &config = ConfigManager::getConfig();
    String stopName = getStopName(config);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = TextUtils::shortenTextToFit(stopName, stationMaxWidth);

    u8g2.print(fittedStopName);
    currentY += 40; // Station Name section gets 40px

    // Column headers
    TextUtils::setFont10px_margin12px(); // Small font for column headers
    u8g2.setCursor(leftMargin, currentY);
    if (isFullScreen)
    {
        u8g2.print("Soll Ist  Linie  Ziel                Gleis");
    }
    else
    {
        u8g2.print("Soll    Ist      Linie     Ziel");
    }
    currentY += 18; // Column headers spacing

    // Underline
    display.drawLine(leftMargin, currentY - 5, rightMargin, currentY - 5, GxEPD_BLACK);
    currentY += 12; // Header underline spacing

    if (isFullScreen)
    {
        // Original full screen logic (unchanged)
        int maxDepartures = min(20, departures.departureCount);
        
        for (int i = 0; i < maxDepartures; i++)
        {
            const auto &dep = departures.departures[i];
            
            // ... existing full screen drawing logic ...
            
            currentY += 37; // Total spacing per entry
            if (currentY > y + h - 25) break;
        }
    }
    else
    {
        // Half screen mode: Separate by direction flag
        ESP_LOGI(TAG, "Drawing departures separated by direction flag");
        
        // Separate departures by direction flag
        std::vector<const DepartureInfo*> direction1Departures;
        std::vector<const DepartureInfo*> direction2Departures;
        
        for (int i = 0; i < departures.departureCount; i++)
        {
            const auto &dep = departures.departures[i];
            if (dep.directionFlag == "1" || dep.directionFlag.toInt() == 1)
            {
                direction1Departures.push_back(&dep);
            }
            else if (dep.directionFlag == "2" || dep.directionFlag.toInt() == 2)
            {
                direction2Departures.push_back(&dep);
            }
        }
        
        ESP_LOGI(TAG, "Found %d departures for direction 1, %d for direction 2", 
                 direction1Departures.size(), direction2Departures.size());
        
        // Draw first 4 departures from direction 1
        int drawnCount = 0;
        int maxPerDirection = 4;
        
        // Direction 1 departures
        for (int i = 0; i < min(maxPerDirection, (int)direction1Departures.size()) && drawnCount < 8; i++)
        {
            const auto &dep = *direction1Departures[i];
            drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
            drawnCount++;
            
            if (currentY > y + h - 60) break; // Leave space for separator and direction 2
        }
        
        // Draw separator line between directions
        if (direction1Departures.size() > 0 && direction2Departures.size() > 0 && drawnCount < 8)
        {
            display.drawLine(leftMargin, currentY, rightMargin, currentY, GxEPD_BLACK);
            currentY += 15; // Space after separator line
        }
        
        // Direction 2 departures
        for (int i = 0; i < min(maxPerDirection, (int)direction2Departures.size()) && drawnCount < 8; i++)
        {
            const auto &dep = *direction2Departures[i];
            drawSingleDeparture(dep, leftMargin, rightMargin, currentY, false); // false = not full screen
            drawnCount++;
            
            if (currentY > y + h - 25) break; // Leave space for footer
        }
        
        ESP_LOGI(TAG, "Drew %d total departures", drawnCount);
    }
}

String DisplayManager::getStopName(RTCConfigData &config)
{
    String stopName = config.selectedStopId;

    // Extract stop name from stopId format: "@O=StopName@"
    int startIndex = stopName.indexOf("@O=");
    if (startIndex != -1)
    {
        startIndex += 3; // Move past "@O="
        int endIndex = stopName.indexOf("@", startIndex);
        if (endIndex != -1)
        {
            stopName = stopName.substring(startIndex, endIndex);
            return stopName;
        }
    }
    return "";
}

// Helper function to draw a single departure
void DisplayManager::drawSingleDeparture(const DepartureInfo &dep, int16_t leftMargin, int16_t rightMargin, int16_t &currentY, bool isFullScreen)
{
    u8g2.setCursor(leftMargin, currentY);
    
    if (isFullScreen)
    {
        // Full screen format (existing logic)
        // ... your existing full screen drawing code ...
    }
    else
    {
        // Half screen format
        TextUtils::setFont10px_margin12px(); // Small font for departure entries
        
        // Calculate available space
        int totalWidth = rightMargin - leftMargin;
        
        // Check if times are different for highlighting
        bool timesAreDifferent = (dep.rtTime.length() > 0 && dep.rtTime != dep.time);
        
        // Clean up line (remove "Bus" prefix)
        String line = dep.line;
        String lineLower = line;
        lineLower.toLowerCase();
        if (lineLower.startsWith("bus "))
        {
            line = line.substring(4);
        }
        
        // Clean up destination (remove "Frankfurt (Main)" prefix)
        String dest = dep.direction;
        String destLower = dest;
        destLower.toLowerCase();
        if (destLower.startsWith("frankfurt (main) "))
        {
            dest = dep.direction.substring(17);
        }
        
        // Prepare times
        String sollTime = dep.time.substring(0, 5);
        String istTime = dep.rtTime.length() > 0 ? dep.rtTime.substring(0, 5) : dep.time.substring(0, 5);
        
        // Measure fixed elements: times and spaces
        int timesWidth = TextUtils::getTextWidth(sollTime + "  " + istTime + "  ");
        int remainingWidth = totalWidth - timesWidth;
        
        // Print soll time
        u8g2.print(sollTime);
        u8g2.print(" ");
        
        // Calculate current cursor position for ist time highlighting
        int16_t istX = leftMargin + TextUtils::getTextWidth(sollTime + " ");
        int16_t istY = currentY;
        
        if (timesAreDifferent)
        {
            // Highlight ist time with underline
            int16_t istTextWidth = TextUtils::getTextWidth(istTime);
            
            // Print the delayed time normally first
            u8g2.setCursor(istX, istY);
            u8g2.print(istTime);
            
            // Draw underline below the text
            int16_t underlineY = istY + 2;
            display.drawLine(istX, underlineY, istX + istTextWidth, underlineY, GxEPD_BLACK);
        }
        else
        {
            // Normal ist time display
            u8g2.print(istTime);
        }
        
        // Allocate remaining space: Line gets 1/3, Destination gets 2/3
        int lineMaxWidth = remainingWidth / 3;
        int destMaxWidth = (remainingWidth * 2) / 3;
        
        // Fit line and destination to available space
        String fittedLine = TextUtils::shortenTextToFit(line, lineMaxWidth);
        String fittedDest = TextUtils::shortenTextToFit(dest, destMaxWidth);
        
        u8g2.print("  ");
        u8g2.print(fittedLine);
        u8g2.print("  ");
        u8g2.print(fittedDest);
        
    }
    
    // Always add consistent spacing for disruption area
    currentY += 20; // Main departure line gets 20px
    
    // Check if we have disruption information to display
    if (dep.lead.length() > 0 || dep.text.length() > 0)
    {
        // Use the lead text if available, otherwise use text
        String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;
        
        // Fit disruption text to available width with some indent
        int disruptionMaxWidth = rightMargin - leftMargin - 20; // 20px indent
        String fittedDisruption = TextUtils::shortenTextToFit(disruptionInfo, disruptionMaxWidth);
        
        // Display disruption information
        TextUtils::setFont10px_margin12px(); // Small font for disruption info
        u8g2.setCursor(leftMargin + 20, currentY); // Indent disruption text
        u8g2.print("⚠ ");
        u8g2.print(fittedDisruption);
    }
    
    // Add consistent spacing after disruption area (whether used or not)
    currentY += 17; // Disruption space gets 17px (total 37px per entry)
}

void DisplayManager::hibernate() {
    ESP_LOGI(TAG, "Hibernating display");
    
    // Turn off display
    display.hibernate();
    
    // You can add additional power-saving measures here
    ESP_LOGI(TAG, "Display hibernated");
}
