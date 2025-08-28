#include "display/weather_general_full.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"
#include "display/display_shared.h"
#include "util/util.h"

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
    // Display current date in DD.MM.YYYY Weekday format
    String dateString = "";
    if (TimeManager::isTimeSet()) {
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char dateStr[30];
            // Format: DD.MM.YYYY Weekday (e.g., "27.08.2025 Tuesday")
            strftime(dateStr, sizeof(dateStr), "%d.%m.%Y %A", &timeinfo);
            dateString = String(dateStr);
        }
    }
    TextUtils::printTextAtWithMargin(leftMargin, currentY, dateString);
    currentY += 60; // Space after date

    // First Column - Weather Icon and Current Temperature
    int colY = currentY; // All columns start at same Y position
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    ESP_LOGI(TAG, "Draw Left Section");
    // Current Weather Icon
    // Get weather icon from weather code using the new utility function
    icon_name currentWeatherIcon = Util::getWeatherIcon(weather.weatherCode);
    display->drawInvertedBitmap(leftMargin, colY, getBitmap(currentWeatherIcon, 64), 64, 64, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(leftMargin + 70, colY, "Weather: " + weather.weatherCode);
    // Current Weather Temperature
    TextUtils::printTextAtWithMargin(leftMargin, colY + 50, "Current Temp." + weather.temperature + "°C");
    // Temperature low high
    TextUtils::printTextAtWithMargin(leftMargin + screenThirdHalfWidth, colY, "Temp.");
    String tempRange = weather.dailyForecast[0].tempMin + " | " + weather.dailyForecast[0].tempMax + "°C";
    TextUtils::printTextAtWithMargin(leftMargin + screenThirdHalfWidth * 2, colY, tempRange);
    // Feels like temperature low high
    TextUtils::printTextAtWithMargin(leftMargin + screenThirdHalfWidth, colY + 50, "Gefühlte");
    String feelTempRange = weather.dailyForecast[0].apparentTempMin + " | " + weather.dailyForecast[0].apparentTempMax +
        "°C";
    TextUtils::printTextAtWithMargin(leftMargin + screenThirdHalfWidth * 2, colY + 50, feelTempRange);
    currentY += 100; // Move down after first row of weather info

    int16_t firstColumn = 0;
    int16_t secondColumn = 70;
    int16_t thirdColumn = 195;
    // Sunrise Sunset
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_sunrise, 64), 64, 64, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY, "Sonnenaufgang");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, weather.dailyForecast[0].sunrise);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY, "Sonnenuntergang");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 20, weather.dailyForecast[0].sunset);
    currentY += 80; // Move down after first row of weather info

    // Sun-shine UN-Index
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_0_day_sunny, 64), 64, 64, GxEPD_BLACK);
    // Convert sunshine duration from seconds to hh:mm format
    String sunshineText = "";
    if (!weather.dailyForecast[0].sunshineDuration.isEmpty()) {
        float sunshineSeconds = weather.dailyForecast[0].sunshineDuration.toFloat();
        int totalMinutes = (int)(sunshineSeconds / 60);
        int hours = totalMinutes / 60;
        int minutes = totalMinutes % 60;
        char timeStr[10];
        sprintf(timeStr, "%02d:%02d", hours, minutes);
        sunshineText += String(timeStr);
    } else {
        sunshineText += "N/A";
    }
    TextUtils::printTextAtWithMargin(secondColumn, currentY, "Sonnenstunden");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, sunshineText);
    // UV Index Low (1-2), Moderate (3-5), High (6-7), Very High (8-10), and Extreme (11+)
    String uvText = "";
    if (!weather.dailyForecast[0].uvIndex.isEmpty()) {
        float uvValue = weather.dailyForecast[0].uvIndex.toFloat();
        String uvGrade;

        if (uvValue <= 2.0) {
            uvGrade = "Low";
        } else if (uvValue <= 5.0) {
            uvGrade = "Moderate";
        } else if (uvValue <= 7.0) {
            uvGrade = "High";
        } else if (uvValue <= 10.0) {
            uvGrade = "Very High";
        } else {
            uvGrade = "Extreme";
        }

        uvText += uvGrade + " (" + weather.dailyForecast[0].uvIndex + ")";
    } else {
        uvText += "N/A";
    }
    TextUtils::printTextAtWithMargin(thirdColumn, currentY, "UV Index");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 20, uvText);
    currentY += 80; // Move down after first row of weather info

    // precipitation mm, precipitation hours
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_61_rain, 64), 64, 64, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(secondColumn, currentY, "Niederschlag (mm)");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, weather.dailyForecast[0].precipitationSum);
    TextUtils::printTextAtWithMargin(thirdColumn, currentY, "Niederschlag Stunden");
    TextUtils::printTextAtWithMargin(thirdColumn, currentY + 20, weather.dailyForecast[0].precipitationHours);
    currentY += 80; // Move down after first row of weather info

    // Wind speed m/s, Wind Gust m/s, Wind Direction
    display->drawInvertedBitmap(firstColumn, currentY, getBitmap(wi_strong_wind, 64), 64, 64, GxEPD_BLACK);
    // Convert wind direction degrees to compass direction
    String windDirectionText = "";
    if (!weather.dailyForecast[0].windDirection.isEmpty()) {
        float windDegrees = weather.dailyForecast[0].windDirection.toFloat();

        // Convert degrees to compass direction
        if (windDegrees >= 348.75 || windDegrees < 11.25) {
            windDirectionText = "N";
        } else if (windDegrees >= 11.25 && windDegrees < 33.75) {
            windDirectionText = "NNE";
        } else if (windDegrees >= 33.75 && windDegrees < 56.25) {
            windDirectionText = "NE";
        } else if (windDegrees >= 56.25 && windDegrees < 78.75) {
            windDirectionText = "ENE";
        } else if (windDegrees >= 78.75 && windDegrees < 101.25) {
            windDirectionText = "E";
        } else if (windDegrees >= 101.25 && windDegrees < 123.75) {
            windDirectionText = "ESE";
        } else if (windDegrees >= 123.75 && windDegrees < 146.25) {
            windDirectionText = "SE";
        } else if (windDegrees >= 146.25 && windDegrees < 168.75) {
            windDirectionText = "SSE";
        } else if (windDegrees >= 168.75 && windDegrees < 191.25) {
            windDirectionText = "S";
        } else if (windDegrees >= 191.25 && windDegrees < 213.75) {
            windDirectionText = "SSW";
        } else if (windDegrees >= 213.75 && windDegrees < 236.25) {
            windDirectionText = "SW";
        } else if (windDegrees >= 236.25 && windDegrees < 258.75) {
            windDirectionText = "WSW";
        } else if (windDegrees >= 258.75 && windDegrees < 281.25) {
            windDirectionText = "W";
        } else if (windDegrees >= 281.25 && windDegrees < 303.75) {
            windDirectionText = "WNW";
        } else if (windDegrees >= 303.75 && windDegrees < 326.25) {
            windDirectionText = "NW";
        } else if (windDegrees >= 326.25 && windDegrees < 348.75) {
            windDirectionText = "NNW";
        }
    } else {
        windDirectionText = "N/A";
    }

    String windText = weather.dailyForecast[0].windSpeedMax + " m/s - "
        + weather.dailyForecast[0].windGustsMax + " m/s "
        + windDirectionText + " (" + weather.dailyForecast[0].windDirection + "°)";

    TextUtils::printTextAtWithMargin(secondColumn, currentY, "Wind");
    TextUtils::printTextAtWithMargin(secondColumn, currentY + 20, windText);
    currentY += 80; // Move down after first row of weather info

    // -----------------------------------
    // Right Side for daily forecast and weather graph
    //-----------------------------------
    ESP_LOGI(TAG, "Draw Right Section");
    currentY = 0; // Reset Y for right side

    // City/Town Name with proper margin
    TextUtils::setFont24px_margin28px();
    // Calculate available width and fit city name
    RTCConfigData& config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - screenHalfWidth;
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, cityMaxWidth);
    TextUtils::printTextAtWithMargin(screenHalfWidth, currentY, fittedCityName); // Use helper function
    currentY += 60; // Space after city name

    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    // Day 1 - 5 Forecast

    for (int i = 1; i < weather.dailyForecastCount && i < 7; i++) {
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY, weather.dailyForecast[i].weatherCode);
        TextUtils::printTextAtWithMargin(screenTenthWidth * (i + 3), currentY + 20,
                                         weather.dailyForecast[i].tempMin + "|" +
                                         weather.dailyForecast[i].tempMax + "°C"
        );
    }
    currentY += 100; // Space after day labels

    TextUtils::setFont12px_margin15px(); // Medium font for graph headers
    TextUtils::printTextAtWithMargin(screenHalfWidth, currentY, "Nächste 12 Stunden");
    currentY += 15; // Nächste 12 Stunden
    currentY += 25; // Space after
    ESP_LOGI(TAG, "Draw Weather Graph");
    // Draw the actual weather graph
    WeatherGraph::drawTemperatureAndRainGraph(weather, screenTenthWidth * 4, currentY,
                                              screenTenthWidth * 6, DisplayShared::getScreenHeight() - currentY);
}

void WeatherFullDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
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
