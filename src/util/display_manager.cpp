#include "util/display_manager.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"
#include "util/time_manager.h"
#include "util/weather_graph.h"

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

    // Define header height
    const int16_t headerHeight = 25;
    const int16_t footerHeight = 15;
    const int16_t contentY = headerHeight;
    const int16_t contentHeight = screenHeight - headerHeight - footerHeight;

    if (weather && departures)
    {
        // Full update - both halves with header
        ESP_LOGI(TAG, "Updating both halves with header");

        bool isFullUpdate = true; // Full update for both halves
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);

            // Draw header across full width
            drawHeaderSection(isFullUpdate, 0, 0, screenWidth, headerHeight);
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

    const int16_t headerHeight = 25;
    const int16_t footerHeight = 25;
    const int16_t contentY = headerHeight;
    const int16_t contentHeight = screenHeight - headerHeight;

    // Landscape: weather is LEFT half (including header portion)
    int16_t x = 0, y = 0, w = halfWidth, h = screenHeight;

    if (!isFullUpate)
    {
        display.setPartialWindow(x, y, w, h);

        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping weather half update in non-full update mode");
        return;
    }
    // Use partial window for faster update
    drawWeatherSection(weather, x, contentY, w, contentHeight);
    drawWeatherFooter(x, screenHeight - footerHeight);
}

void DisplayManager::updateDepartureHalf(bool isFullUpate,const DepartureData &departures)
{
    ESP_LOGI(TAG, "Updating departure half");

    const int16_t headerHeight = 25;
    const int16_t footerHeight = 25;
    const int16_t contentY = headerHeight;
    const int16_t contentHeight = screenHeight - headerHeight;

    // Landscape: departures are RIGHT half (no header in this section)
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
        drawWeatherSection(weather, 0, 0, screenWidth, screenHeight);
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

void DisplayManager::drawWeatherSection(const WeatherInfo &weather, int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t currentY = y + 25; // Start after header space
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    // City/Town Name: 40px
    setMediumFont();
    u8g2.setCursor(leftMargin, currentY);

    // Calculate available width and fit city name
    RTCConfigData &config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - leftMargin;
    String fittedCityName = shortenTextToFit(config.cityName, cityMaxWidth);
    u8g2.print(fittedCityName);
    currentY += 40;

    if (isFullScreen)
    {
        // Day weather Info section: 67px total
        // Calculate column widths
        int columnWidth = (rightMargin - leftMargin) / 3;
        int firstColX = leftMargin;
        int secondColX = leftMargin + columnWidth;
        int thirdColX = leftMargin + (2 * columnWidth);

        // First Column - Weather Icon and Current Temperature
        int colY = currentY; // All columns start at same Y position

        // Day Weather Icon: 37px
        setLargeFont();
        u8g2.setCursor(firstColX, colY);
        u8g2.print(weather.weatherCode); // Weather condition as text
        colY += 37;

        // Current Temperature: 30px
        setLargeFont();
        u8g2.setCursor(firstColX, colY);
        u8g2.print(weather.temperature);
        u8g2.print("°C");

        // Second Column - Today's temps, UV, Pollen
        colY = currentY; // Reset to baseline Y for second column

        // Today low/high temp: 27px
        setMediumFont();
        u8g2.setCursor(secondColX, colY);
        u8g2.print(weather.dailyForecast[0].tempMin);
        u8g2.print(" | ");
        u8g2.print(weather.dailyForecast[0].tempMax);
        u8g2.print("°C");
        colY += 13;
        colY += 14; // Total 27px for high/low

        // UV Index info: 20px
        setSmallFont();
        u8g2.setCursor(secondColX, colY);
        u8g2.print("UV Index: ");
        u8g2.print(weather.dailyForecast[0].uvIndex);
        colY += 20;

        // Third Column - Date, Sunrise, Sunset
        colY = currentY; // Reset to baseline Y for third column

        // Date Info: 27px
        setMediumFont();
        u8g2.setCursor(thirdColX, colY);
        u8g2.print("Today");
        colY += 13;
        u8g2.setCursor(thirdColX, colY);
        // Add current date if available
        u8g2.print("Juli 13"); // Placeholder - should use actual date
        colY += 14;            // Total 27px

        // Sunrise: 20px
        setSmallFont();
        u8g2.setCursor(thirdColX, colY);
        u8g2.print("Sunrise: ");
        u8g2.print(weather.dailyForecast[0].sunrise);
        colY += 20;

        // Sunset: 20px
        u8g2.setCursor(thirdColX, colY);
        u8g2.print("Sunset: ");
        u8g2.print(weather.dailyForecast[0].sunset);

        currentY += 67; // Move past the day weather info section

        // Weather Graphic section: 333px
        int graphicY = currentY;
        int graphicHeight = 333;

        // Draw the actual weather graph
        WeatherGraph::drawTemperatureAndRainGraph(weather,
                                                  leftMargin, graphicY,
                                                  rightMargin - leftMargin, graphicHeight);

        currentY += graphicHeight; // Move past the graphic section
    }
    else
    {

        // Draw fist Column - Current Temperature and Condition
        int16_t dayWeatherInfoY = currentY;

        // Each Column has a fixed height of 67px

        // Current weather icon: 37px
        // Current temperature: 30px
        wertherInfoFirstColumn(leftMargin, currentY, weather);

        int16_t currentX = leftMargin + 100;
        // Draw second Column - Today's temps, UV, Pollen
        // Today's low/high temp: 27px
        // Today's UV Indexinfo: 20px
        // Today's Pollen info: 20px
        weatherInfoSecondColumn(currentX, dayWeatherInfoY, weather);
        currentX += 150; // Move to next column

        // Draw third Column - Date, Sunrise, Sunset
        // Date Info: 27px
        // Sunrise: 20px
        // Sunset: 20px
        weatherInfoThirdColumn(currentX, dayWeatherInfoY, weather);

        // Weather Graph section (replaces text-based forecast for better visualization)
        setSmallFont();
        u8g2.setCursor(leftMargin, currentY);
        u8g2.print("Next 12 Hours:");
        currentY += 18;

        // Calculate available space for graph
        int availableHeight = (y + h - 15) - currentY; // Leave 15px for footer
        int graphHeight = min(333, availableHeight);   // Max 333px, but adapt to available space

        if (graphHeight >= 80)
        { // Only draw graph if we have enough space
            WeatherGraph::drawTemperatureAndRainGraph(weather,
                                                      leftMargin, currentY,
                                                      rightMargin - leftMargin, graphHeight);
            currentY += graphHeight;
        }
        else
        {
            // Fallback to compact text forecast if not enough space for graph
            int maxForecast = min(6, availableHeight / 16); // Limit based on available space
            for (int i = 0; i < min(maxForecast, weather.hourlyForecastCount); i++)
            {
                const auto &forecast = weather.hourlyForecast[i];
                u8g2.setCursor(leftMargin, currentY);

                String timeStr = forecast.time.substring(11, 16); // HH:MM
                u8g2.print(timeStr);
                u8g2.print(" ");
                u8g2.print(forecast.temperature);
                u8g2.print("° ");
                u8g2.print(forecast.rainChance);
                u8g2.print("%");

                currentY += 16;
                if (currentY > y + h - 40)
                    break; // Leave space for footer
            }
        }
    }
}

void DisplayManager::weatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather)
{
    setSmallFont();
    u8g2.setCursor(currentX, dayWeatherInfoY);
    u8g2.print("Date :");
    u8g2.print("Juli 13"); // Placeholder - should use actual date

    u8g2.setCursor(currentX, dayWeatherInfoY + 27);
    u8g2.print("Sunrise : ");
    u8g2.print(weather.dailyForecast[0].sunrise);

    u8g2.setCursor(currentX, dayWeatherInfoY + 47);
    u8g2.print("Sunset : ");
    u8g2.print(weather.dailyForecast[0].sunset);
}

void DisplayManager::weatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather)
{
    setSmallFont();
    u8g2.setCursor(currentX, dayWeatherInfoY);
    u8g2.print(weather.dailyForecast[0].tempMin);
    u8g2.print(" | ");
    u8g2.print(weather.dailyForecast[0].tempMax);

    u8g2.setCursor(currentX, dayWeatherInfoY + 27);
    u8g2.print("UV Index : ");
    u8g2.print(weather.dailyForecast[0].uvIndex);

    u8g2.setCursor(currentX, dayWeatherInfoY + 47);
    u8g2.print("Pollen : ");
    u8g2.print("N/A");
}

void DisplayManager::wertherInfoFirstColumn(int16_t leftMargin, int16_t &currentY, const WeatherInfo &weather)
{
    setSmallFont();

    // Draw first Column - Current Temperature and Condition
    u8g2.setCursor(leftMargin, currentY);
    // weather_code is missing
    // u8g2.print(weather.coded);
    // Day weather Info section: 37px total
    // Todo Add weather icon support
    currentY += 47;

    // Current temperature: 30px
    u8g2.setCursor(leftMargin, currentY);
    u8g2.print(weather.temperature);
    u8g2.print("°C  ");
    currentY += 20;
}

void DisplayManager::drawDepartureFooter(int16_t x, int16_t y)
{
    setSmallFont();

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

void DisplayManager::drawWeatherFooter(int16_t x, int16_t y)
{
    setSmallFont();

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
    int16_t currentY = y + 25;
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    // Station name
    setMediumFont();
    u8g2.setCursor(leftMargin, currentY);
    RTCConfigData &config = ConfigManager::getConfig();
    String stopName = getStopName(config);

    // Calculate available width and fit station name
    int stationMaxWidth = rightMargin - leftMargin;
    String fittedStopName = shortenTextToFit(stopName, stationMaxWidth);

    u8g2.print(fittedStopName);
    currentY += 40; // Updated: Station Name section gets 40px

    // Column headers
    u8g2.setCursor(leftMargin, currentY);
    if (isFullScreen)
    {
        u8g2.print("Soll Ist  Linie  Ziel                Gleis");
    }
    else
    {
        setSmallFont();
        // 5 char for soll, one space, 5 char for ist, one space, 4 char for line, 20 char for destination
        u8g2.print("Soll    Ist      Linie     Ziel");
    }
    currentY += 18; // Column headers spacing

    // Underline
    display.drawLine(leftMargin, currentY - 5, rightMargin, currentY - 5, GxEPD_BLACK);
    currentY += 12; // Updated: Header underline spacing (18 + 12 = 30px total for headers)

    // Departures
    int maxDepartures = isFullScreen ? 20 : 10;
    maxDepartures = min(maxDepartures, departures.departureCount);

    for (int i = 0; i < maxDepartures; i++)
    {
        const auto &dep = departures.departures[i];

        u8g2.setCursor(leftMargin, currentY);

        // Format for available width
        if (isFullScreen)
        {
            // Full screen format: "Soll Ist  Linie  Ziel                Gleis"
            setSmallFont(); // Set font for measurements

            // Calculate available space for each column
            int totalWidth = rightMargin - leftMargin;

            // Fixed widths for times and track
            String sollTime = dep.time.substring(0, 5);
            String istTime = dep.rtTime.length() > 0 ? dep.rtTime.substring(0, 5) : dep.time.substring(0, 5);
            String track = dep.track.substring(0, 4);

            // Check if times are different for highlighting
            bool timesAreDifferent = (dep.rtTime.length() > 0 && dep.rtTime != dep.time);

            // Measure fixed elements
            int sollWidth = getTextWidth(sollTime + " ");
            int istWidth = getTextWidth(istTime + "  ");
            int trackWidth = getTextWidth("  " + track);

            // Available space for line and destination
            int remainingWidth = totalWidth - sollWidth - istWidth - trackWidth;

            // Allocate space: Line gets 1/4, Destination gets 3/4
            int lineMaxWidth = remainingWidth / 4;
            int destMaxWidth = (remainingWidth * 3) / 4;

            // Fit line and destination to available space
            String line = shortenTextToFit(dep.line, lineMaxWidth);
            String dest = shortenTextToFit(dep.direction, destMaxWidth);

            // Print soll time
            u8g2.print(sollTime);
            u8g2.print(" ");

            // Calculate current cursor position for ist time highlighting
            int16_t istX = leftMargin + getTextWidth(sollTime + " ");
            int16_t istY = currentY;

            if (timesAreDifferent)
            {
                // Highlight ist time with black background and white text
                int16_t istTextWidth = getTextWidth(istTime);
                int16_t textHeight = 12; // Approximate text height for small font

                // Draw black background rectangle
                display.fillRect(istX, istY - textHeight + 2, istTextWidth, textHeight, GxEPD_BLACK);

                // Set white text color
                display.setTextColor(GxEPD_WHITE);
                u8g2.setCursor(istX, istY);
                u8g2.print(istTime);

                // Reset to black text color
                display.setTextColor(GxEPD_BLACK);
            }
            else
            {
                // Normal ist time display
                u8g2.print(istTime);
            }

            u8g2.print("  ");
            u8g2.print(line);
            u8g2.print("  ");
            u8g2.print(dest);
            u8g2.print("  ");
            u8g2.print(track);
        }
        else
        {
            // Half screen format: "Soll Ist  Linie  Ziel"
            setSmallFont(); // Set font for measurements

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
            int timesWidth = getTextWidth(sollTime + "  " + istTime + "  ");
            int remainingWidth = totalWidth - timesWidth;

            // Print soll time
            u8g2.print(sollTime);
            u8g2.print(" ");

            // Calculate current cursor position for ist time highlighting
            int16_t istX = leftMargin + getTextWidth(sollTime + " ");
            int16_t istY = currentY;

            if (timesAreDifferent)
            {
                // Highlight ist time with underline instead of rectangle
                int16_t istTextWidth = getTextWidth(istTime);

                // Print the delayed time normally first
                u8g2.setCursor(istX, istY);
                u8g2.print(istTime);

                // Draw underline below the text
                int16_t underlineY = istY + 2; // Position underline 2px below text baseline
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
            String fittedLine = shortenTextToFit(line, lineMaxWidth);
            String fittedDest = shortenTextToFit(dest, destMaxWidth);

            u8g2.print("  ");
            u8g2.print(fittedLine);
            u8g2.print("  ");
            u8g2.print(fittedDest);
            u8g2.print("  ");
            u8g2.print(dep.directionFlag);
        }

        // Always add consistent spacing for disruption area
        currentY += 20; // Updated: Main departure line gets 20px

        // Check if we have disruption information to display
        if (dep.lead.length() > 0 || dep.text.length() > 0)
        {
            // Use the lead text if available, otherwise use text
            String disruptionInfo = dep.lead.length() > 0 ? dep.lead : dep.text;

            // Fit disruption text to available width with some indent
            int disruptionMaxWidth = rightMargin - leftMargin - 20; // 20px indent
            String fittedDisruption = shortenTextToFit(disruptionInfo, disruptionMaxWidth);

            // Display disruption information
            setSmallFont();
            u8g2.setCursor(leftMargin + 20, currentY); // Indent disruption text
            u8g2.print("⚠ ");                          // Warning symbol
            u8g2.print(fittedDisruption);
        }
        // If no disruption info, the space is left empty but still allocated

        // Add consistent spacing after disruption area (whether used or not)
        currentY += 17; // Updated: Disruption space gets 17px (total 37px per entry)

        if (currentY > y + h - 25)
            break;
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

void DisplayManager::clearRegion(DisplayRegion region)
{
    int16_t x, y, w, h;
    getRegionBounds(region, x, y, w, h);

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
    } while (display.nextPage());
}

void DisplayManager::getRegionBounds(DisplayRegion region, int16_t &x, int16_t &y, int16_t &w, int16_t &h)
{
    switch (region)
    {
    case DisplayRegion::FULL_SCREEN:
        x = 0;
        y = 0;
        w = screenWidth;
        h = screenHeight;
        break;

    // Landscape regions
    case DisplayRegion::LEFT_HALF:
        x = 0;
        y = 0;
        w = halfWidth;
        h = screenHeight;
        break;
    case DisplayRegion::RIGHT_HALF:
        x = halfWidth;
        y = 0;
        w = halfWidth;
        h = screenHeight;
        break;

    // Semantic regions (landscape only)
    case DisplayRegion::WEATHER_AREA:
        // Weather is in LEFT half in landscape
        getRegionBounds(DisplayRegion::LEFT_HALF, x, y, w, h);
        break;
    case DisplayRegion::DEPARTURE_AREA:
        // Departures is in RIGHT half in landscape
        getRegionBounds(DisplayRegion::RIGHT_HALF, x, y, w, h);
        break;
    }
}

void DisplayManager::hibernate()
{
    display.hibernate();
    ESP_LOGI(TAG, "Display hibernated");
}

void DisplayManager::setLargeFont()
{
    u8g2.setFont(u8g2_font_helvB18_tf); // 18pt Helvetica Bold with German support
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::setMediumFont()
{
    u8g2.setFont(u8g2_font_helvB12_tf); // 12pt Helvetica Bold with German support
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::setSmallFont()
{
    u8g2.setFont(u8g2_font_helvB10_tf); // 10pt Helvetica Bold with German support
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);
}

// Text width measurement helpers
int16_t DisplayManager::getTextWidth(const String &text)
{
    return u8g2.getUTF8Width(text.c_str());
}

int16_t DisplayManager::getTextExcess(const String &text, int16_t maxWidth)
{
    int16_t textWidth = getTextWidth(text);
    return max(0, textWidth - maxWidth); // Return excess pixels, or 0 if fits
}

String DisplayManager::shortenTextToFit(const String &text, int16_t maxWidth)
{
    if (getTextWidth(text) <= maxWidth)
    {
        return text; // Already fits
    }

    // Binary search to find the longest text that fits
    int left = 0;
    int right = text.length();
    String result = "";

    while (left <= right)
    {
        int mid = (left + right) / 2;
        String candidate = text.substring(0, mid);

        // Add "..." if we're shortening
        if (mid < text.length())
        {
            candidate += "...";
        }

        if (getTextWidth(candidate) <= maxWidth)
        {
            result = candidate;
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return result.length() > 0 ? result : "..."; // Fallback to "..." if nothing fits
}

// Helper function for wrapping long text
void DisplayManager::printWrappedText(const String &text, int16_t x, int16_t &y, int16_t maxWidth, int16_t maxChars, int16_t lineHeight)
{
    if (text.length() <= maxChars)
    {
        // Text fits on one line
        u8g2.setCursor(x, y);
        u8g2.print(text);
    }
    else
    {
        // Text needs wrapping
        String firstLine = text.substring(0, maxChars - 3) + "...";
        u8g2.setCursor(x, y);
        u8g2.print(firstLine);

        // Check if we have space for a second line
        if (y + lineHeight < screenHeight - 25)
        {
            y += lineHeight;
            u8g2.setCursor(x + 20, y); // Slight indent for continuation
            String secondLine = text.substring(maxChars - 3);
            if (secondLine.length() > maxChars)
            {
                secondLine = secondLine.substring(0, maxChars - 3) + "...";
            }
            u8g2.print(secondLine);
        }
    }
}

void DisplayManager::drawHeaderSection(bool isFullUpate, int16_t x, int16_t y, int16_t w, int16_t h)
{
    ESP_LOGI(TAG, "=== drawHeaderSection called ===");
    ESP_LOGI(TAG, "Input parameters: x=%d, y=%d, w=%d, h=%d", x, y, w, h);
    ESP_LOGI(TAG, "Screen dimensions: %dx%d", screenWidth, screenHeight);

    // Set up partial window for header area only
    if (!isFullUpate)
    {
        // Use partial window for faster update
        display.setPartialWindow(x, y, w, h);
        // Clear header area first
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Using partial window for header: (%d, %d, %d, %d)", x, y, w, h);
    }

    ESP_LOGI(TAG, "Header area cleared");

    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;
    int16_t centerY = y + h / 2 + 5; // Center vertically in header area

    ESP_LOGI(TAG, "Calculated margins: left=%d, right=%d, centerY=%d", leftMargin, rightMargin, centerY);

    setSmallFont();

    // WiFi status placeholder (left side)
    ESP_LOGI(TAG, "Drawing WiFi status at (%d, %d)", leftMargin, centerY);
    u8g2.setCursor(leftMargin, centerY);
    u8g2.print("WiFi: [●●●]"); // Placeholder for WiFi strength
    ESP_LOGI(TAG, "WiFi status printed");

    // Battery status placeholder (right side)
    String batteryText = "Batt: [85%]"; // Placeholder for battery level
    int16_t batteryWidth = getTextWidth(batteryText);
    ESP_LOGI(TAG, "Battery text: '%s', width: %d", batteryText.c_str(), batteryWidth);

    int16_t batteryX = rightMargin - batteryWidth;
    ESP_LOGI(TAG, "Drawing battery status at (%d, %d)", batteryX, centerY);

    if (batteryX < leftMargin)
    {
        ESP_LOGW(TAG, "Battery text position %d overlaps with WiFi text (leftMargin: %d)", batteryX, leftMargin);
    }

    u8g2.setCursor(batteryX, centerY);
    u8g2.print(batteryText);
    ESP_LOGI(TAG, "Battery status printed");

    // Add separator line at bottom of header
    int16_t lineY = y + h - 1;
    ESP_LOGI(TAG, "Drawing separator line from (%d, %d) to (%d, %d)", x, lineY, x + w, lineY);

    display.drawLine(x, lineY, x + w, lineY, GxEPD_BLACK);
}
