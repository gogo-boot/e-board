#include "display/weather_display.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>

static const char *TAG = "WEATHER_DISPLAY";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *WeatherDisplay::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX *WeatherDisplay::u8g2 = nullptr;
int16_t WeatherDisplay::screenWidth = 0;
int16_t WeatherDisplay::screenHeight = 0;

void WeatherDisplay::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                         U8G2_FOR_ADAFRUIT_GFX &u8g2Ref, 
                         int16_t screenW, int16_t screenH) {
    display = &displayRef;
    u8g2 = &u8g2Ref;
    screenWidth = screenW;
    screenHeight = screenH;
    
    ESP_LOGI(TAG, "WeatherDisplay initialized with screen size %dx%d", screenW, screenH);
}

void WeatherDisplay::drawWeatherSection(const WeatherInfo &weather, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "WeatherDisplay not initialized! Call init() first.");
        return;
    }
    
    int16_t currentY = y; // Start from top edge
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;

    bool isFullScreen = (w >= screenWidth * 0.8);

    if (isFullScreen) {
        drawFullScreenWeatherLayout(weather, leftMargin, rightMargin, currentY, y, h);
    } else {
        drawHalfScreenWeatherLayout(weather, leftMargin, rightMargin, currentY, y, h);
    }
}

void WeatherDisplay::drawFullScreenWeatherLayout(const WeatherInfo &weather, 
                                                int16_t leftMargin, int16_t rightMargin, 
                                                int16_t &currentY, int16_t y, int16_t h) {

    // City/Town Name with proper margin
    TextUtils::setFont24px_margin28px(); 

    // Calculate available width and fit city name
    RTCConfigData &config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - leftMargin;
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, cityMaxWidth);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, fittedCityName); // Use helper function
    currentY += 28 + 10; // Space after city name

    // Day weather Info section
    // Calculate column widths
    int columnWidth = (rightMargin - leftMargin) / 3;
    int firstColX = leftMargin;
    int secondColX = leftMargin + columnWidth;
    int thirdColX = leftMargin + (2 * columnWidth);

    // First Column - Weather Icon and Current Temperature
    int colY = currentY; // All columns start at same Y position

    // Day Weather Icon
    TextUtils::setFont18px_margin22px(); // Large font for weather display
    colY += 22; // Apply margin
    TextUtils::printTextAtWithMargin(firstColX, colY - 22, weather.weatherCode);
    colY += 15; // Space after weather code

    // Current Temperature
    TextUtils::setFont18px_margin22px(); // Large font for temperature
    colY += 22; // Apply margin
    String tempText = weather.temperature + "°C";
    TextUtils::printTextAtWithMargin(firstColX, colY - 22, tempText);

    // Second Column - Today's temps, UV, Pollen
    colY = currentY + 22; // Reset to baseline Y for second column with margin

    // Today low/high temp
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    String tempRange = weather.dailyForecast[0].tempMin + " | " + weather.dailyForecast[0].tempMax + "°C";
    TextUtils::printTextAtWithMargin(secondColX, colY - 15, tempRange);
    colY += 27; // Space for next item

    // UV Index info
    TextUtils::setFont10px_margin12px(); // Small font for UV info
    String uvText = "UV Index: " + String(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(secondColX, colY - 12, uvText);
    colY += 20;

    // Third Column - Date, Sunrise, Sunset
    colY = currentY; // Reset to baseline Y for third column

    // Date Info: 27px
    TextUtils::setFont12px_margin15px(); // Medium font for date info
    String todayText = "Today";
    TextUtils::printTextAtWithMargin(thirdColX, colY, todayText);
    colY += 13;
    String dateText = "Juli 13"; // Placeholder - should use actual date
    TextUtils::printTextAtWithMargin(thirdColX, colY, dateText);
    colY += 14;            // Total 27px

    // Sunrise: 20px
    TextUtils::setFont10px_margin12px(); // Small font for sunrise/sunset
    String sunriseText = "Sunrise: " + String(weather.dailyForecast[0].sunrise);
    TextUtils::printTextAtWithMargin(thirdColX, colY, sunriseText);
    colY += 20;

    // Sunset: 20px
    String sunsetText = "Sunset: " + String(weather.dailyForecast[0].sunset);
    TextUtils::printTextAtWithMargin(thirdColX, colY, sunsetText);

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

void WeatherDisplay::drawHalfScreenWeatherLayout(const WeatherInfo &weather, 
                                                int16_t leftMargin, int16_t rightMargin, 
                                                int16_t &currentY, int16_t y, int16_t h) {
    // City/Town Name with proper margin
    TextUtils::setFont14px_margin17px(); 

    // Parse ISO date string (e.g., "2025-07-16T15:30") and format as "01. July"
    String isoTime = weather.time;
    int year = 0, month = 0, day = 0;
    if (isoTime.length() >= 10) {
        year = isoTime.substring(0, 4).toInt();
        month = isoTime.substring(5, 7).toInt();
        day = isoTime.substring(8, 10).toInt();
    }
    static const char* monthNames[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    String dateText = "";
    if (month > 0 && month <= 12 && day > 0) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d. %s", day, monthNames[month]);
        dateText = buf;
    } else {
        dateText = "Date: N/A";
    }
    int16_t dateTextWidth = TextUtils::getTextWidth(dateText); // Ensure the text is measured
    TextUtils::printTextAtWithMargin(rightMargin - dateTextWidth, currentY, dateText);

    TextUtils::setFont18px_margin22px(); 
    // Calculate available width and fit city name
    RTCConfigData &config = ConfigManager::getConfig();
    int16_t maxCityWidth = rightMargin - leftMargin - dateTextWidth - 20; // Reserve space for date text
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, maxCityWidth);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, fittedCityName); // Use helper function

    TextUtils::setFont14px_margin17px(); 

    currentY += 22; // city name
    currentY += 20; // Space after city name

    int16_t maxWidth = rightMargin - leftMargin;
    int16_t columnWidth = maxWidth / 3;

    // Each Column has a fixed height of 67px
    drawWeatherInfoFirstColumn(leftMargin, currentY, weather);

    // Draw second Column - Today's temps, UV, Pollen
    int16_t currentX = leftMargin + columnWidth;
    drawWeatherInfoSecondColumn(currentX, currentY, weather);

    // Draw third Column - Date, Sunrise, Sunset
    currentX += columnWidth;
    drawWeatherInfoThirdColumn(currentX, currentY, weather);

    currentY += 80; // Day Weather Information section height
    currentY += 12; // Space after Day Weather Information section

    // Weather Graph section (replaces text-based forecast for better visualization)
    TextUtils::setFont12px_margin15px(); // Medium font for graph headers
    TextUtils::printTextAtWithMargin(leftMargin, currentY, "Nächste 12 Stunden");
    currentY += 15; // Nächste 12 Stunden 
    currentY += 25; // Space after 

    // Calculate available space for graph
    int availableHeight = (y + h - 15) - currentY; // Leave 15px for footer
    int graphHeight = min(304, availableHeight);   // Max 307px, but adapt to available space

    // Only draw graph if we have enough space
    WeatherGraph::drawTemperatureAndRainGraph(weather,
                                                leftMargin, currentY,
                                                rightMargin - leftMargin, graphHeight);
    currentY += graphHeight;
}

void WeatherDisplay::drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t dayWeatherInfoY, const WeatherInfo &weather) {
    TextUtils::setFont10px_margin12px(); // Small font for weather info

    // Draw first Column - Current Temperature and Condition
    // weather_code is missing
    // TextUtils::printTextAtWithMargin(leftMargin, dayWeatherInfoY, weather.coded); // Uncomment if available
    // Day weather Info section: 37px total
    // Todo Add weather icon support

    // Current temperature: 30px
    String tempText = String(weather.temperature) + "°C  ";
    TextUtils::printTextAtWithMargin(leftMargin, dayWeatherInfoY + 47, tempText);
}

void WeatherDisplay::drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather) {
    TextUtils::setFont12px_margin15px(); // Small font for weather info
    String tempRange = String(weather.dailyForecast[0].tempMin) + "°C | " + String(weather.dailyForecast[0].tempMax) + "°C";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY, tempRange);

    TextUtils::setFont10px_margin12px(); // Small font for weather info
    String uvText = "UV Index : " + String(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 27, uvText);

    String pollenText = "Pollen : N/A";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 47, pollenText);
}

void WeatherDisplay::drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather) {

    int8_t padding = 30;
    currentX += padding; // Add padding to the left
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    display->drawInvertedBitmap( currentX, dayWeatherInfoY + 27 , getBitmap(wi_sunrise, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 27, weather.dailyForecast[0].sunrise);

    display->drawInvertedBitmap( currentX, dayWeatherInfoY + 47, getBitmap(wi_sunset, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 47, weather.dailyForecast[0].sunset);
}

void WeatherDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
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
