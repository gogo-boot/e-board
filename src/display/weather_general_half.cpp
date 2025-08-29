#include "display/weather_general_half.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"
#include "display/display_shared.h"
#include "util/date_util.h"
#include "util/weather_util.h"

static const char* TAG = "WEATHER_DISPLAY";

void WeatherHalfDisplay::drawHalfScreenWeatherLayout(const WeatherInfo& weather,
                                                     int16_t leftMargin, int16_t rightMargin,
                                                     int16_t y, int16_t h) {
    ESP_LOGI(TAG, "drawHalfScreenWeatherLayout called with margins (%d,%d) and area (%d,%d)", leftMargin, rightMargin,
             y, h);
    int16_t currentY = y; // Start from top edge

    // City/Town Name with proper margin
    TextUtils::setFont14px_margin17px();

    // Use Util::formatDateText to get the formatted date text
    String dateText = WeatherUtil::formatDateText(weather.time);
    int16_t dateTextWidth = TextUtils::getTextWidth(dateText); // Ensure the text is measured
    TextUtils::printTextAtWithMargin(rightMargin - dateTextWidth, currentY, dateText);

    TextUtils::setFont18px_margin22px();
    // Calculate available width and fit city name
    RTCConfigData& config = ConfigManager::getConfig();
    int16_t maxCityWidth = rightMargin - leftMargin - dateTextWidth - 20; // Reserve space for date text
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, maxCityWidth);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, fittedCityName); // Use helper function

    TextUtils::setFont14px_margin17px();

    currentY += 22; // city name
    currentY += 20; // Space after city name

    int16_t maxWidth = rightMargin - leftMargin;

    // Each Column has a fixed height of 67px
    drawWeatherInfoFirstColumn(leftMargin, currentY, weather);

    // Draw second Column - Today's temps, UV, Pollen
    int16_t currentX = leftMargin + 80;
    drawWeatherInfoSecondColumn(currentX, currentY, weather);

    // Draw third Column - Date, Sunrise, Sunset
    currentX += 170;
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
    int graphHeight = min(304, availableHeight); // Max 307px, but adapt to available space

    // Only draw graph if we have enough space
    WeatherGraph::drawTemperatureAndRainGraph(weather,
                                              leftMargin, currentY,
                                              rightMargin - leftMargin, graphHeight);
    currentY += graphHeight;
}

void WeatherHalfDisplay::drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t dayWeatherInfoY,
                                                    const WeatherInfo& weather) {
    TextUtils::setFont10px_margin12px(); // Small font for weather info

    // Draw first Column - Current Temperature and Condition
    // Draw weather icon using Util::getWeatherIcon
    icon_name currentWeatherIcon = WeatherUtil::getWeatherIcon(weather.weatherCode);
    auto* display = DisplayShared::getDisplay();
    display->drawInvertedBitmap(leftMargin, dayWeatherInfoY, getBitmap(currentWeatherIcon, 48), 48, 48, GxEPD_BLACK);
    // Current temperature: 30px
    String tempText = String(weather.temperature) + "°C  ";
    TextUtils::printTextAtWithMargin(leftMargin, dayWeatherInfoY + 47, tempText);

    // Draw date string using Util::getDateText
    String dateText = DateUtil::formatDateText(weather.time); // Use WeatherUtil for weather-related utils
    TextUtils::printTextAtWithMargin(leftMargin, dayWeatherInfoY + 65, dateText);
}

void WeatherHalfDisplay::drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                     const WeatherInfo& weather) {
    TextUtils::setFont12px_margin15px(); // Small font for weather info
    String tempRange = String(weather.dailyForecast[0].tempMin) + "°C - " + String(weather.dailyForecast[0].tempMax) +
        "°C";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY, tempRange);

    // apply UV index to grade conversion
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    String uvText = "UV Index : " + WeatherUtil::uvIndexToGrade(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 27, uvText);

    // Show wind speed in "min - max m/s" format using Util
    String windText = "Wind : " + weather.dailyForecast[0].windSpeedMax + " m/s";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 47, windText);
}

void WeatherHalfDisplay::drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                    const WeatherInfo& weather) {
    int8_t padding = 30;
    currentX += padding; // Add padding to the left
    TextUtils::setFont10px_margin12px(); // Small font for weather info

    auto* display = DisplayShared::getDisplay();
    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 15, getBitmap(wi_sunrise, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 40, dayWeatherInfoY + 27, weather.dailyForecast[0].sunrise);

    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 35, getBitmap(wi_sunset, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 40, dayWeatherInfoY + 47, weather.dailyForecast[0].sunset);
}

void WeatherHalfDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
    auto* display = DisplayShared::getDisplay();
    auto* u8g2 = DisplayShared::getU8G2();

    if (!display || !u8g2) {
        ESP_LOGE(TAG, "Display not initialized! Call DisplayShared::init() first.");
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
    display->drawInvertedBitmap(footerX + timeStrWidth + 5, y, getBitmap(refresh, 16), 16, 16, GxEPD_BLACK);
}
