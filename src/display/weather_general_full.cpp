#include "display/weather_general_full.h"
#include "display/display_manager.h"   // for DisplayConstants::FOOTER_HEIGHT
#include "display/text_utils.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>

#include "config/config_manager.h"
#include "util/weather_util.h"
#include "util/date_util.h"
#include "display/common_footer.h"
#include "global_instances.h"
#include "util/indoor_sensor.h"

static const char* TAG = "WEATHER_DISPLAY";

// Weather full screen display layout constants
namespace WeatherFullDisplayConstants {
    constexpr int16_t SIDE_MARGIN = 10;
    constexpr int16_t DATE_SECTION_HEIGHT = 60;
    constexpr int16_t CITY_NAME_HEIGHT = 40;
    constexpr int16_t WEATHER_ROW_HEIGHT = 75;
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


using namespace WeatherFullDisplayConstants;

void WeatherFullDisplay::drawFullScreenWeatherLayout(const WeatherInfo& weather) {
    // [WEATHER_DISPLAY] drawFullScreenWeatherLayout called with margins (10,790) and area (0,480)
    ESP_LOGI(TAG, "drawFullScreenWeatherLayout called");

    int16_t currentY = 0; // Start from top edge
    int16_t screenWidth = display.width();

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
    // Indoor temperature and humidity (SHT4x sensor, E1001 only)
#ifdef BOARD_S3_E1001
    if (IndoorSensor::isAvailable()) {
        TextUtils::printTextAtWithMargin(100, colY + 80, "Innen");
        String indoorText = String(IndoorSensor::getTemperature(), 1) + "°C  "
                          + String((int)IndoorSensor::getHumidity()) + "%";
        TextUtils::printTextAtWithMargin(screenQuaterWidth, colY + 80, indoorText);
    }
#endif
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
    if (weather.dailyForecast[0].uvIndex > 0) {
        String uvText = WeatherUtil::uvIndexToGrade(weather.dailyForecast[0].uvIndex);
        TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_TITLE, "UV Index");
        TextUtils::printTextAtWithMargin(thirdColumn, currentY + TEXT_Y_VALUE, uvText);
    }
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
                                              display.height() - currentY);
}

void WeatherFullDisplay::drawDayBrowseLayout(const WeatherInfo& weather,
                                              const WeatherHourlyForecast dayHourly[],
                                              int hourlyCount,
                                              int selectedDay) {
    ESP_LOGI(TAG, "drawDayBrowseLayout: day %d, %d hourly entries", selectedDay, hourlyCount);

    int16_t screenWidth = display.width();
    int16_t screenHeight = display.height();
    int16_t leftMargin = SIDE_MARGIN;
    int16_t rightMargin = screenWidth - SIDE_MARGIN;
    int16_t currentY = 0;

    // Validate selectedDay
    if (selectedDay < 0 || selectedDay >= weather.dailyForecastCount) {
        ESP_LOGE(TAG, "Invalid selectedDay %d (count=%d)", selectedDay, weather.dailyForecastCount);
        return;
    }

    const WeatherDailyForecast& dayForecast = weather.dailyForecast[selectedDay];

    // ── Top section (~30px): Date left-aligned + city name right-aligned ──
    TextUtils::setFont24px_margin28px();

    // Format date: "Do, 27. Aug 2026" from dailyForecast[selectedDay].time
    String dayName = DateUtil::getDayOfWeekFromDateString(dayForecast.time, 3); // 3-char day
    String dateText = DateUtil::formatDateText(dayForecast.time);
    String headerDate = dayName + ", " + dateText;
    TextUtils::printTextAtWithMargin(leftMargin, currentY, headerDate);

    // City name right-aligned
    RTCConfigData& config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - (screenWidth / 2);
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, cityMaxWidth);
    int cityNameWidth = TextUtils::getTextWidth(fittedCityName);
    int cityNameX = rightMargin - cityNameWidth;
    TextUtils::printTextAtWithMargin(cityNameX, currentY, fittedCityName);
    currentY += 35; // Space after header

    // ── 6-day forecast row (~100px): Days 1-6 with highlight on selected ──
    TextUtils::setFont12px_margin15px();
    int forecastCount = weather.dailyForecastCount;
    // Calculate column width distributed evenly across actual available days
    int16_t availableWidth = rightMargin - leftMargin;
    int displayDays = forecastCount > 1 ? forecastCount - 1 : 1; // days 1..N-1
    int16_t colWidth = availableWidth / displayDays;

    for (int i = 1; i < forecastCount && i <= 6; i++) {
        int16_t colX = leftMargin + (i - 1) * colWidth;

        // Highlight selected day with rectangle border
        if (i == selectedDay) {
            int16_t rectX = colX - 3;
            int16_t rectY = currentY - 3;
            int16_t rectW = colWidth + 2;
            int16_t rectH = 95 + 6;
            display.drawRect(rectX, rectY, rectW, rectH, GxEPD_BLACK);
            display.drawRect(rectX + 1, rectY + 1, rectW - 2, rectH - 2, GxEPD_BLACK);
        }

        // Day label (2-char)
        String dayLabel = WeatherUtil::getDayOfWeekFromDateString(weather.dailyForecast[i].time, 2);
        TextUtils::printTextAtWithMargin(colX, currentY, dayLabel);

        // Weather icon
        icon_name icon = WeatherUtil::getWeatherIcon(weather.dailyForecast[i].weatherCode);
        display.drawInvertedBitmap(colX, currentY + 15, getBitmap(icon, 48), 48, 48, GxEPD_BLACK);

        // Temp range
        int tempMinInt = (int)weather.dailyForecast[i].tempMin;
        int tempMaxInt = (int)weather.dailyForecast[i].tempMax;
        TextUtils::printTextAtWithMargin(colX, currentY + 70,
                                         String(tempMinInt) + " / " + String(tempMaxInt) + "°");
    }
    currentY += FORECAST_ROW_HEIGHT;

    // ── Graph title ──
    TextUtils::setFont12px_margin15px();
    TextUtils::printTextAtWithMargin(leftMargin, currentY, "Stundenverlauf 06:00 - 24:00");
    currentY += GRAPH_TITLE_HEIGHT;
    currentY += 10; // Small spacing before graph

    // ── Full-width graph (remaining space minus footer) ──
    int16_t footerY = screenHeight - DisplayConstants::FOOTER_HEIGHT;
    int16_t graphH = footerY - currentY - 5; // 5px padding before footer
    int16_t graphW = rightMargin - leftMargin;

    WeatherGraph::drawTemperatureAndRainGraph(dayHourly, hourlyCount,
                                              leftMargin, currentY, graphW, graphH);
}

void WeatherFullDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
    // Use common footer with time and refresh icon
    CommonFooter::drawFooter(x, y, h, FOOTER_TIME | FOOTER_REFRESH | FOOTER_WIFI | FOOTER_BATTERY);
}
