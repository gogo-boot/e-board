#include "display/weather_general_full.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>
#include <icons.h>
#include "config/config_manager.h"

static const char* TAG = "WEATHER_DISPLAY";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* WeatherFullDisplay::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX* WeatherFullDisplay::u8g2 = nullptr;
int16_t WeatherFullDisplay::screenWidth = 0;
int16_t WeatherFullDisplay::screenHeight = 0;

void WeatherFullDisplay::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& displayRef,
                              U8G2_FOR_ADAFRUIT_GFX& u8g2Ref,
                              int16_t screenW, int16_t screenH) {
    display = &displayRef;
    u8g2 = &u8g2Ref;
    screenWidth = screenW;
    screenHeight = screenH;

    ESP_LOGI(TAG, "WeatherDisplay initialized with screen size %dx%d", screenW, screenH);
}

void WeatherFullDisplay::drawFullScreenWeatherLayout(const WeatherInfo& weather,
                                                     int16_t leftMargin, int16_t rightMargin,
                                                     int16_t y, int16_t h) {
    int16_t currentY = y; // Start from top edge
    // City/Town Name with proper margin
    TextUtils::setFont24px_margin28px();

    // Calculate available width and fit city name
    RTCConfigData& config = ConfigManager::getConfig();
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
    colY += 14; // Total 27px

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
    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 27, getBitmap(wi_sunrise, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 27, weather.dailyForecast[0].sunrise);

    display->drawInvertedBitmap(currentX, dayWeatherInfoY + 47, getBitmap(wi_sunset, 32), 32, 32, GxEPD_BLACK);
    TextUtils::printTextAtWithMargin(currentX + 32, dayWeatherInfoY + 47, weather.dailyForecast[0].sunset);
}

void WeatherFullDisplay::drawWeatherFooter(int16_t x, int16_t y, int16_t h) {
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
    display->drawInvertedBitmap(footerX + timeStrWidth + 5, y, getBitmap(refresh, 16), 16, 16, GxEPD_BLACK);
}
