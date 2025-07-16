#include "display/weather_display.h"
#include "display/text_utils.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"
#include <esp_log.h>

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

    // City/Town Name with proper margin
    TextUtils::setFont12px_margin15px(); // 12px font needs 15px margin from top
    currentY += 15; // Apply the margin

    // Calculate available width and fit city name
    RTCConfigData &config = ConfigManager::getConfig();
    int cityMaxWidth = rightMargin - leftMargin;
    String fittedCityName = TextUtils::shortenTextToFit(config.cityName, cityMaxWidth);
    TextUtils::printTextAtWithMargin(leftMargin, currentY - 15, fittedCityName); // Use helper function
    currentY += 25; // Space after city name

    if (isFullScreen) {
        drawFullScreenWeatherLayout(weather, leftMargin, rightMargin, currentY, y, h);
    } else {
        drawHalfScreenWeatherLayout(weather, leftMargin, rightMargin, currentY, y, h);
    }
}

void WeatherDisplay::drawFullScreenWeatherLayout(const WeatherInfo &weather, 
                                                int16_t leftMargin, int16_t rightMargin, 
                                                int16_t &currentY, int16_t y, int16_t h) {
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
    u8g2->setCursor(thirdColX, colY);
    u8g2->print("Today");
    colY += 13;
    u8g2->setCursor(thirdColX, colY);
    // Add current date if available
    u8g2->print("Juli 13"); // Placeholder - should use actual date
    colY += 14;            // Total 27px

    // Sunrise: 20px
    TextUtils::setFont10px_margin12px(); // Small font for sunrise/sunset
    u8g2->setCursor(thirdColX, colY);
    u8g2->print("Sunrise: ");
    u8g2->print(weather.dailyForecast[0].sunrise);
    colY += 20;

    // Sunset: 20px
    u8g2->setCursor(thirdColX, colY);
    u8g2->print("Sunset: ");
    u8g2->print(weather.dailyForecast[0].sunset);

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
    // Draw weather info columns
    int16_t dayWeatherInfoY = currentY;

    // Each Column has a fixed height of 67px
    drawWeatherInfoFirstColumn(leftMargin, currentY, weather);

    int16_t currentX = leftMargin + 100;
    // Draw second Column - Today's temps, UV, Pollen
    drawWeatherInfoSecondColumn(currentX, dayWeatherInfoY, weather);
    currentX += 150; // Move to next column

    // Draw third Column - Date, Sunrise, Sunset
    drawWeatherInfoThirdColumn(currentX, dayWeatherInfoY, weather);

    // Weather Graph section (replaces text-based forecast for better visualization)
    TextUtils::setFont10px_margin12px(); // Small font for graph headers
    u8g2->setCursor(leftMargin, currentY);
    u8g2->print("Next 12 Hours:");
    currentY += 18;

    // Calculate available space for graph
    int availableHeight = (y + h - 15) - currentY; // Leave 15px for footer
    int graphHeight = min(333, availableHeight);   // Max 333px, but adapt to available space

    if (graphHeight >= 80) { 
        // Only draw graph if we have enough space
        WeatherGraph::drawTemperatureAndRainGraph(weather,
                                                  leftMargin, currentY,
                                                  rightMargin - leftMargin, graphHeight);
        currentY += graphHeight;
    } else {
        // Fallback to compact text forecast if not enough space for graph
        drawCompactTextForecast(weather, leftMargin, currentY, y, h, availableHeight);
    }
}

void WeatherDisplay::drawCompactTextForecast(const WeatherInfo &weather, 
                                           int16_t leftMargin, int16_t &currentY, 
                                           int16_t y, int16_t h, int availableHeight) {
    int maxForecast = min(6, availableHeight / 16); // Limit based on available space
    for (int i = 0; i < min(maxForecast, weather.hourlyForecastCount); i++) {
        const auto &forecast = weather.hourlyForecast[i];
        u8g2->setCursor(leftMargin, currentY);

        String timeStr = forecast.time.substring(11, 16); // HH:MM
        u8g2->print(timeStr);
        u8g2->print(" ");
        u8g2->print(forecast.temperature);
        u8g2->print("° ");
        u8g2->print(forecast.rainChance);
        u8g2->print("% ");
        u8g2->print(forecast.humidity); // Add humidity display
        u8g2->print("%RH");

        currentY += 16;
        if (currentY > y + h - 40) break; // Leave space for footer
    }
}

void WeatherDisplay::drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t &currentY, const WeatherInfo &weather) {
    TextUtils::setFont10px_margin12px(); // Small font for weather info

    // Draw first Column - Current Temperature and Condition
    u8g2->setCursor(leftMargin, currentY);
    // weather_code is missing
    // u8g2->print(weather.coded);
    // Day weather Info section: 37px total
    // Todo Add weather icon support
    currentY += 47;

    // Current temperature: 30px
    u8g2->setCursor(leftMargin, currentY);
    u8g2->print(weather.temperature);
    u8g2->print("°C  ");
    currentY += 20;
}

void WeatherDisplay::drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather) {
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    u8g2->setCursor(currentX, dayWeatherInfoY);
    u8g2->print(weather.dailyForecast[0].tempMin);
    u8g2->print(" | ");
    u8g2->print(weather.dailyForecast[0].tempMax);

    u8g2->setCursor(currentX, dayWeatherInfoY + 27);
    u8g2->print("UV Index : ");
    u8g2->print(weather.dailyForecast[0].uvIndex);

    u8g2->setCursor(currentX, dayWeatherInfoY + 47);
    u8g2->print("Pollen : ");
    u8g2->print("N/A");
}

void WeatherDisplay::drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather) {
    TextUtils::setFont10px_margin12px(); // Small font for weather info
    u8g2->setCursor(currentX, dayWeatherInfoY);
    u8g2->print("Date :");
    u8g2->print("Juli 13"); // Placeholder - should use actual date

    u8g2->setCursor(currentX, dayWeatherInfoY + 27);
    u8g2->print("Sunrise : ");
    u8g2->print(weather.dailyForecast[0].sunrise);

    u8g2->setCursor(currentX, dayWeatherInfoY + 47);
    u8g2->print("Sunset : ");
    u8g2->print(weather.dailyForecast[0].sunset);
}

void WeatherDisplay::drawWeatherFooter(int16_t x, int16_t y) {
    if (!display || !u8g2) {
        ESP_LOGE(TAG, "WeatherDisplay not initialized! Call init() first.");
        return;
    }
    
    TextUtils::setFont10px_margin12px(); // Small font for footer

    // Ensure footer is positioned properly within bounds
    int16_t footerY = min(y, (int16_t)(screenHeight - 20)); // Ensure at least 20px from bottom
    int16_t footerX = x + 10;
    ESP_LOGI(TAG, "Weather footer position: (%d, %d)", footerX, footerY);

    u8g2->setCursor(footerX, footerY); // Add 10px left margin
    u8g2->print("Aktualisiert: ");

    // Check if time is properly set
    if (TimeManager::isTimeSet()) {
        // Get current German time using TimeManager
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            char timeStr[20];
            // German time format: "HH:MM DD.MM."
            strftime(timeStr, sizeof(timeStr), "%H:%M %d.%m.", &timeinfo);
            u8g2->print(timeStr);
        } else {
            u8g2->print("Zeit nicht verfügbar");
        }
    } else {
        // Time not synchronized
        u8g2->print("Zeit nicht synchronisiert");
    }
}
