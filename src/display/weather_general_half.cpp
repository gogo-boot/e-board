#include "display/weather_general_half.h"
#include "display/text_utils.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"
#include "util/weather_util.h"
#include "display/common_footer.h"
#include "global_instances.h"

static const char* TAG = "WEATHER_DISPLAY";

// Weather display layout constants
namespace WeatherDisplayConstants {
    constexpr int16_t CITY_NAME_HEIGHT = 22;
    constexpr int16_t CITY_NAME_SPACING = 20;
    constexpr int16_t CITY_DATE_SPACING = 20;
    constexpr int16_t FIRST_COLUMN_WIDTH = 80;
    constexpr int16_t SECOND_COLUMN_WIDTH = 170;
    constexpr int16_t WEATHER_INFO_HEIGHT = 80;
    constexpr int16_t SECTION_SPACING = 12;
    constexpr int16_t GRAPH_HEADER_HEIGHT = 15;
    constexpr int16_t GRAPH_HEADER_SPACING = 25;
    constexpr int16_t FOOTER_HEIGHT = 15;
    constexpr int16_t MAX_GRAPH_HEIGHT = 304;
    constexpr int16_t WEATHER_ICON_SIZE = 48;
}


using namespace WeatherDisplayConstants;

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
    int16_t maxCityWidth = rightMargin - leftMargin - dateTextWidth - CITY_DATE_SPACING;
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, maxCityWidth);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, fittedCityName);

    TextUtils::setFont14px_margin17px();

    currentY += CITY_NAME_HEIGHT;
    currentY += CITY_NAME_SPACING;

    int16_t maxWidth = rightMargin - leftMargin;

    // Each Column has a fixed height of 67px
    drawWeatherInfoFirstColumn(leftMargin, currentY, weather);

    // Draw second Column - Today's temps, UV, Pollen
    int16_t currentX = leftMargin + FIRST_COLUMN_WIDTH;
    drawWeatherInfoSecondColumn(currentX, currentY, weather);

    // Draw third Column - Date, Sunrise, Sunset
    currentX += SECOND_COLUMN_WIDTH;
    drawWeatherInfoThirdColumn(currentX, currentY, weather);

    currentY += WEATHER_INFO_HEIGHT;
    currentY += SECTION_SPACING;

    // Weather Graph section (replaces text-based forecast for better visualization)
    TextUtils::setFont12px_margin15px();
    TextUtils::printTextAtWithMargin(leftMargin, currentY, "Nächste 12 Stunden");
    currentY += GRAPH_HEADER_HEIGHT;
    currentY += GRAPH_HEADER_SPACING;

    // Calculate available space for graph
    int16_t availableHeight = (y + h - FOOTER_HEIGHT) - currentY;
    int16_t graphHeight = min(MAX_GRAPH_HEIGHT, availableHeight);

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
    display.drawInvertedBitmap(leftMargin, dayWeatherInfoY, getBitmap(currentWeatherIcon, WEATHER_ICON_SIZE),
                               WEATHER_ICON_SIZE, WEATHER_ICON_SIZE, GxEPD_BLACK);
    // Current temperature: 30px
    String tempText = String(weather.temperature, 1) + "°C  ";
    TextUtils::printTextAtWithMargin(leftMargin, dayWeatherInfoY + 47, tempText);
}

void WeatherHalfDisplay::drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                     const WeatherInfo& weather) {
    TextUtils::setFont12px_margin15px(); // Small font for weather info
    String tempRange = String(weather.dailyForecast[0].tempMin, 0) + "°C / " + String(
            weather.dailyForecast[0].tempMax, 0)
        +
        "°C";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY, tempRange);

    // apply UV index to grade conversion
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    String uvText = "UV Index : " + WeatherUtil::uvIndexToGrade(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 27, uvText);

    // Show wind speed in "min - max m/s" format using Util
    String windDirectionText = WeatherUtil::degreeToCompass(weather.dailyForecast[0].windDirection);
    String windText = "Wind : Max " + String(weather.dailyForecast[0].windSpeedMax, 0) + " m/s ( " + windDirectionText +
        " )";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 47, windText);
}

void WeatherHalfDisplay::drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                    const WeatherInfo& weather) {
    int8_t padding = 30;
    currentX += padding; // Add padding to the left
    TextUtils::setFont10px_margin12px(); // Small font for weather info

    display.drawInvertedBitmap(currentX, dayWeatherInfoY + 15, getBitmap(wi_sunrise, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 40, dayWeatherInfoY + 27, weather.dailyForecast[0].sunrise);

    display.drawInvertedBitmap(currentX, dayWeatherInfoY + 35, getBitmap(wi_sunset, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 40, dayWeatherInfoY + 47, weather.dailyForecast[0].sunset);
}

void WeatherHalfDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_WIFI | FOOTER_BATTERY);
}
