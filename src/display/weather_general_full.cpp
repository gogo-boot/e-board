#include "display/weather_general_full.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"
#include "display/display_shared.h"

static const char* TAG = "WEATHER_DISPLAY";

void WeatherFullDisplay::drawFullScreenWeatherLayout(const WeatherInfo& weather,
                                                     int16_t leftMargin, int16_t rightMargin,
                                                     int16_t y, int16_t h) {
    // [WEATHER_DISPLAY] drawFullScreenWeatherLayout called with margins (10,790) and area (0,480)
    ESP_LOGI(TAG, "drawFullScreenWeatherLayout called with margins (%d,%d) and area (%d,%d)", leftMargin, rightMargin,
             y, h);

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
    currentY += 28 + 10; // Space after date

    // First Column - Weather Icon and Current Temperature
    int colY = currentY; // All columns start at same Y position
    TextUtils::setFont12px_margin15px(); // Medium font for temp range
    ESP_LOGI(TAG, "Draw Left Section");
    // Current Weather Icon
    TextUtils::printTextAtWithMargin(leftMargin, colY, "Weather Code" + weather.weatherCode);
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
    // Sunrise Sunset
    String sunriseText = "Sunrise: " + String(weather.dailyForecast[0].sunrise);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, sunriseText);
    String sunsetText = "Sunset: " + String(weather.dailyForecast[0].sunset);
    TextUtils::printTextAtWithMargin(screenTenthWidth * 3, currentY, sunsetText);
    currentY += 50; // Move down after first row of weather info
    // Sun-shine UN-Index
    // Convert sunshine duration from seconds to hh:mm format
    String sunshineText = "Sun-shine: ";
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
    TextUtils::printTextAtWithMargin(leftMargin, currentY, sunshineText);
    // Todo apply UV Index Grade
    // UV Index Low (1-2), Moderate (3-5), High (6-7), Very High (8-10), and Extreme (11+)
    String uvText = "UV Index: " + String(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(screenTenthWidth * 3, currentY, uvText);
    currentY += 50; // Move down after first row of weather info
    // precipitation mm, precipitation hours
    String precipitationText = "Regen mm: " + String(weather.dailyForecast[0].precipitationSum);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, precipitationText);
    String precipitationHoursText = "Regen Std: " + String(weather.dailyForecast[0].precipitationHours);
    TextUtils::printTextAtWithMargin(screenTenthWidth * 3, currentY, precipitationHoursText);
    currentY += 50; // Move down after first row of weather info
    // Wind speed m/s, Wind Gust m/s
    String windSpeedText = "Wind: " + String(weather.dailyForecast[0].windSpeedMax);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, windSpeedText);
    String windGustText = "Wind Böen: " + String(weather.dailyForecast[0].windGustsMax);
    TextUtils::printTextAtWithMargin(screenTenthWidth * 3, currentY, windGustText);
    currentY += 50; // Move down after first row of weather info
    // Wind Direction Arrow
    String windDirectionText = "Wind Richtung: " + String(weather.dailyForecast[0].windDirection);
    TextUtils::printTextAtWithMargin(leftMargin, currentY, windDirectionText);

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
    currentY += 28 + 10; // Space after city name

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

void WeatherFullDisplay::drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t dayWeatherInfoY,
                                                    const WeatherInfo& weather) {
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

void WeatherFullDisplay::drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                     const WeatherInfo& weather) {
    TextUtils::setFont12px_margin15px(); // Small font for weather info
    String tempRange = String(weather.dailyForecast[0].tempMin) + "°C | " + String(weather.dailyForecast[0].tempMax) +
        "°C";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY, tempRange);

    TextUtils::setFont10px_margin12px(); // Small font for weather info
    String uvText = "UV Index : " + String(weather.dailyForecast[0].uvIndex);
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 27, uvText);

    String pollenText = "Pollen : N/A";
    TextUtils::printTextAtWithMargin(currentX, dayWeatherInfoY + 47, pollenText);
}

void WeatherFullDisplay::drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY,
                                                    const WeatherInfo& weather) {
    int8_t padding = 30;
    currentX += padding; // Add padding to the left
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    auto* display = DisplayShared::getDisplay();
    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 27, getBitmap(wi_sunrise, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 27, weather.dailyForecast[0].sunrise);

    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 47, getBitmap(wi_sunset, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 47, weather.dailyForecast[0].sunset);
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
