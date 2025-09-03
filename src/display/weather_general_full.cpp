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

void WeatherFullDisplay::drawFullScreenWeatherLayout(const WeatherInfo& weather,
                                                     int16_t leftMargin, int16_t rightMargin,
                                                     int16_t y, int16_t h) {
    // [WEATHER_DISPLAY] drawFullScreenWeatherLayout called with margins (10,790) and area (0,480)
    ESP_LOGI(TAG, "drawFullScreenWeatherLayout called with margins (%d,%d) and area (%d,%d)", leftMargin, rightMargin,
             y, h);
    auto* display = DisplayShared::getDisplay();

    int16_t currentY = y; // Start from top edge
    int16_t screenWidth = DisplayShared::getScreenWidth();
    // int16_t screenWidth = 400;
    int16_t screenHalfWidth = screenWidth / 2;
    int16_t screenQuaterWidth = screenWidth / 4;
    int16_t screenTenthWidth = screenWidth / 10;
    int16_t screenThirdHalfWidth = screenHalfWidth / 3;

    //-----------------------------------
    // Left Side for current weather info
    //-----------------------------------
    TextUtils::setFont24px_margin28px();
    // Display current date in DD.MM.YYYY Weekday format using DateUtil
    String dateString = DateUtil::getCurrentDateString(); // or use another field if needed
    TextUtils::printTextAtWithMargin(leftMargin, currentY, dateString);
    currentY += 60; // Space after date

    // First Column - Weather Icon and Current Temperature
    int colY = currentY; // All columns start at same Y position
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    ESP_LOGI(TAG, "Draw Left Section");
    // Current Weather Icon
    // Get weather icon from weather code using the new utility function
    icon_name currentWeatherIcon = WeatherUtil::getWeatherIcon(weather.weatherCode);
    display->drawInvertedBitmap(leftMargin, colY, getBitmap(currentWeatherIcon, 64), 64, 64, GxEPD_BLACK);
    // Current Weather Temperature
    TextUtils::setFont14px_margin17px();
    TextUtils::printTextAtWithMargin(leftMargin, colY + 70, weather.temperature + "°C");
    // Temperature low high
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    TextUtils::printTextAtWithMargin(100, colY + 20, "Temp.");
    String tempRange = weather.dailyForecast[0].tempMin + " - " + weather.dailyForecast[0].tempMax + "°C";
    TextUtils::printTextAtWithMargin(screenQuaterWidth, colY + 20, tempRange);
    // Feels like temperature low high
    TextUtils::printTextAtWithMargin(100, colY + 50, "Gefühlte");
    String feelTempRange = weather.dailyForecast[0].apparentTempMin + " - " + weather.dailyForecast[0].apparentTempMax +
        "°C";
    TextUtils::printTextAtWithMargin(screenQuaterWidth, colY + 50, feelTempRange);
    currentY += 100; // Move down after first row of weather info

    int16_t firstColumn = 0;
    int16_t secondColumn = 70;
    int16_t thirdColumn = 195;
    // Sunrise Sunset
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_sunrise, 64), 64, 64, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, "Sonnenauf / untergang");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 40, weather.dailyForecast[0].sunrise);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 40, weather.dailyForecast[0].sunset);
    currentY += 80; // Move down after first row of weather info

    // Sun-shine UN-Index
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_0_day_sunny, 64), 64, 64, GxEPD_BLACK);
    // Use Util::sunshineSecondsToHHMM for sunshine duration
    String sunshineText = WeatherUtil::sunshineSecondsToHHMM(weather.dailyForecast[0].sunshineDuration);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, "Sonnenstd.");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 40, sunshineText);
    // Use Util::uvIndexToGrade for UV Index
    String uvText = WeatherUtil::uvIndexToGrade(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 20, "UV Index");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 40, uvText);
    currentY += 80; // Move down after first row of weather info

    // precipitation mm, precipitation hours
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_61_rain, 64), 64, 64, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, "Niederschlag");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 40, weather.dailyForecast[0].precipitationSum + " mm");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 20, "Dauer");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 40, weather.dailyForecast[0].precipitationHours + " Std");
    currentY += 80; // Move down after first row of weather info

    // Wind speed m/s, Wind Gust m/s, Wind Direction
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_strong_wind, 64), 64, 64, GxEPD_BLACK);
    // Use Util::degreeToCompass for wind direction
    String windDirectionText = WeatherUtil::degreeToCompass(weather.dailyForecast[0].windDirection.toFloat());
    String windText = weather.dailyForecast[0].windSpeedMax + " m/s (Böe "
        + weather.dailyForecast[0].windGustsMax + " m/s )";
    String windText2 = windDirectionText + " (" + weather.dailyForecast[0].windDirection + "°)";
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, "Wind " + windText2);
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 40, windText);
    currentY += 80; // Move down after first row of weather info

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
    currentY += 40; // Space after city name

    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    // Day 1 - 5 Forecast

    for (int i = 1; i < weather.dailyForecastCount; i++) {
        // YYYY-MM-DD to Day of week
        String dayLabel = WeatherUtil::getDayOfWeekFromDateString(weather.dailyForecast[i].time, 2);
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY, dayLabel);

        // Draw WMO weather icon for each day using Util::getWeatherIcon
        icon_name icon = WeatherUtil::getWeatherIcon(weather.dailyForecast[i].weatherCode);
        display->drawInvertedBitmap(screenTenthWidth * (i + 3), currentY + 15, getBitmap(icon, 64), 64, 64,
                                    GxEPD_BLACK);

        // Show low | high temp without floating point
        int tempMinInt = (int)weather.dailyForecast[i].tempMin.toFloat();
        int tempMaxInt = (int)weather.dailyForecast[i].tempMax.toFloat();
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY + 75,
                                         String(tempMinInt) + " - " + String(tempMaxInt) + "°");
    }
    currentY += 100; // Space after day labels

    TextUtils::setFont12px_margin15px(); // Medium font for graph headers
    TextUtils::printTextAtWithMargin(screenTenthWidth * 4, currentY, "Nächste 12 Stunden");
    currentY += 15; // Nächste 12 Stunden
    currentY += 25; // Space after
    ESP_LOGI(TAG, "Draw Weather Graph");
    // Draw the actual weather graph
    WeatherGraph::drawTemperatureAndRainGraph(weather, screenTenthWidth * 4, currentY,
                                              screenTenthWidth * 6, DisplayShared::getScreenHeight() - currentY);
}

void WeatherFullDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_WIFI);
}
