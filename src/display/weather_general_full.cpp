#include "display/weather_general_full.h"
#include "display/text_utils.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"
#include "display/display_shared.h"
#include "util/weather_util.h"
#include "util/date_util.h"
#include "display/common_footer.h"

static const char* TAG = "WEATHER_DISPLAY";

// Weather full screen display layout constants
namespace WeatherFullDisplayConstants {
    constexpr int16_t SIDE_MARGIN = 10;
    constexpr int16_t DATE_SECTION_HEIGHT = 60;
    constexpr int16_t CITY_NAME_HEIGHT = 40;
    constexpr int16_t WEATHER_ROW_HEIGHT = 80;
    constexpr int16_t FORECAST_ROW_HEIGHT = 100;
    constexpr int16_t GRAPH_TITLE_HEIGHT = 15;
    constexpr int16_t GRAPH_SPACING = 25;

    // Icon sizes
    constexpr int16_t LARGE_ICON = 64;

    // Left side column positions
    constexpr int16_t COL1_X = 0;
    constexpr int16_t COL2_X = 70;
    constexpr int16_t COL3_X = 195;

    // Text Y offsets within rows
    constexpr int16_t TEXT_Y_TITLE = 20;
    constexpr int16_t TEXT_Y_VALUE = 40;
    constexpr int16_t TEMP_TEXT_Y = 70;
}

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

using namespace WeatherFullDisplayConstants;

void WeatherFullDisplay::drawFullScreenWeatherLayout(const WeatherInfo& weather) {
    // [WEATHER_DISPLAY] drawFullScreenWeatherLayout called with margins (10,790) and area (0,480)
    ESP_LOGI(TAG, "drawFullScreenWeatherLayout called");

    int16_t currentY = 0; // Start from top edge
    int16_t screenWidth = DisplayShared::getScreenWidth();

    int16_t leftMargin = SIDE_MARGIN;
    int16_t rightMargin = screenWidth - SIDE_MARGIN;
    int16_t screenHalfWidth = screenWidth / 2;
    int16_t screenQuaterWidth = screenWidth / 4;
    int16_t screenTenthWidth = screenWidth / 10;
    int16_t screenThirdHalfWidth = screenHalfWidth / 3;

    //-----------------------------------
    // Left Side for current weather info
    //-----------------------------------
    TextUtils::setFont24px_margin28px();
    // Display current date in DD.MM.YYYY Weekday format using DateUtil
    String dateString = DateUtil::getCurrentDateString();
    TextUtils::printTextAtWithMargin(leftMargin, currentY, dateString);
    currentY += DATE_SECTION_HEIGHT; // Space after date

    // First Column - Weather Icon and Current Temperature
    int colY = currentY; // All columns start at same Y position
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    ESP_LOGI(TAG, "Draw Left Section");
    // Current Weather Icon
    // Get weather icon from weather code using the new utility function
    icon_name currentWeatherIcon = WeatherUtil::getWeatherIcon(weather.weatherCode);
    display.drawInvertedBitmap(leftMargin, colY, getBitmap(currentWeatherIcon, LARGE_ICON), LARGE_ICON, LARGE_ICON,
                               GxEPD_BLACK);
    // Current Weather Temperature
    TextUtils::setFont14px_margin17px();
    TextUtils::printTextAtWithMargin(leftMargin, colY + TEMP_TEXT_Y, String(weather.temperature, 1) + "°C");
    // Temperature low high
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    TextUtils::printTextAtWithMargin(100, colY + 20, "Temp.");
    String tempRange = String(weather.dailyForecast[0].tempMin, 0) + " / " + String(weather.dailyForecast[0].tempMax, 0)
        + "°C";
    TextUtils::printTextAtWithMargin(screenQuaterWidth, colY + 20, tempRange);
    // Feels like temperature low high
    TextUtils::printTextAtWithMargin(100, colY + 50, "Gefühlte");
    String feelTempRange = String(weather.dailyForecast[0].apparentTempMin, 0) + " / " + String(
        weather.dailyForecast[0].apparentTempMax, 0) + "°C";
    TextUtils::printTextAtWithMargin(screenQuaterWidth, colY + 50, feelTempRange);
    currentY += 100; // Move down after first row of weather info

    int16_t firstColumn = 0;
    int16_t secondColumn = 70;
    int16_t thirdColumn = 195;

    // Sunrise Sunset
    display.drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_sunrise, LARGE_ICON), LARGE_ICON, LARGE_ICON,
                               GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_TITLE, "Sonnenauf / untergang");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_VALUE, weather.dailyForecast[0].sunrise);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_VALUE, weather.dailyForecast[0].sunset);
    currentY += WEATHER_ROW_HEIGHT; // Move down after first row of weather info

    // Use Util::sunshineSecondsToHHMM for sunshine duration
    // Sun-shine UN-Index
    display.drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_0_day_sunny, LARGE_ICON), LARGE_ICON, LARGE_ICON,
                               GxEPD_BLACK);
    String sunshineText = WeatherUtil::sunshineSecondsToHHMM(weather.dailyForecast[0].sunshineDuration);
    // Use Util::uvIndexToGrade for UV Index
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_TITLE, "Sonnenstd.");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_VALUE, sunshineText);
    String uvText = WeatherUtil::uvIndexToGrade(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_TITLE, "UV Index");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_VALUE, uvText);
    currentY += WEATHER_ROW_HEIGHT; // Move down after first row of weather info

    // precipitation mm, precipitation hours
    display.drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_61_rain, LARGE_ICON), LARGE_ICON, LARGE_ICON,
                               GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_TITLE, "Niederschlag");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_VALUE,
                                     String(weather.dailyForecast[0].precipitationSum, 1) + " mm");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_TITLE, "Dauer");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_VALUE,
                                     String(weather.dailyForecast[0].precipitationHours) + " Std");
    currentY += WEATHER_ROW_HEIGHT; // Move down after first row of weather info

    // Wind speed m/s, Wind Gust m/s, Wind Direction
    display.drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_strong_wind, LARGE_ICON), LARGE_ICON, LARGE_ICON,
                               GxEPD_BLACK);
    String windDirectionText = WeatherUtil::degreeToCompass(weather.dailyForecast[0].windDirection);
    String windText = String(weather.dailyForecast[0].windSpeedMax, 1) + " m/s (Böe " + String(
        weather.dailyForecast[0].windGustsMax, 1) + " m/s )";
    String windText2 = windDirectionText + " (" + String(weather.dailyForecast[0].windDirection) + "°)";
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_TITLE, "Wind " + windText2);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + TEXT_Y_VALUE, windText);
    currentY += WEATHER_ROW_HEIGHT; // Move down after first row of weather info

    // -----------------------------------
    // Right Side for daily forecast and weather graph
    //-----------------------------------
    ESP_LOGI(TAG, "Draw Right Section");
    currentY = 0; // Reset Y for right side

    // City/Town Name with proper margin, right aligned
    TextUtils::setFont24px_margin28px();
    RTCConfigData& config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - screenHalfWidth;
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, cityMaxWidth);
    int cityNameWidth = TextUtils::getTextWidth(fittedCityName);
    int cityNameX = rightMargin - cityNameWidth;
    TextUtils::printTextAtWithMargin(cityNameX, currentY, fittedCityName);
    currentY += CITY_NAME_HEIGHT; // Space after city name

    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    for (int i = 1; i < weather.dailyForecastCount; i++) {
        // YYYY-MM-DD to Day of week
        String dayLabel = WeatherUtil::getDayOfWeekFromDateString(weather.dailyForecast[i].time, 2);
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY, dayLabel);

        // Draw WMO weather icon for each day using Util::getWeatherIcon
        icon_name icon = WeatherUtil::getWeatherIcon(weather.dailyForecast[i].weatherCode);
        display.drawInvertedBitmap(screenTenthWidth * (i + 3), currentY + 15, getBitmap(icon, LARGE_ICON), LARGE_ICON,
                                   LARGE_ICON,
                                   GxEPD_BLACK);

        // Show low | high temp without floating point
        int tempMinInt = (int)weather.dailyForecast[i].tempMin;
        int tempMaxInt = (int)weather.dailyForecast[i].tempMax;
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY + 75,
                                         String(tempMinInt) + " / " + String(tempMaxInt) + "°");
    }
    currentY += FORECAST_ROW_HEIGHT; // Space after day labels

    TextUtils::setFont12px_margin15px(); // Medium font for graph headers
    TextUtils::printTextAtWithMargin(screenTenthWidth * 4, currentY, "Nächste 12 Stunden");
    currentY += GRAPH_TITLE_HEIGHT; // Nächste 12 Stunden
    currentY += GRAPH_SPACING; // Space after
    ESP_LOGI(TAG, "Draw Weather Graph");
    // Draw the actual weather graph
    WeatherGraph::drawTemperatureAndRainGraph(weather, screenTenthWidth * 4, currentY, screenTenthWidth * 6,
                                              DisplayShared::getScreenHeight() - currentY);
}

void WeatherFullDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_WIFI | FOOTER_BATTERY);
}
