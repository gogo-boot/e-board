#include "util/weather_print.h"
#include <Arduino.h>
#include <esp_log.h>

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/pins.h"
#include "config/config_manager.h"

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;

static const char* TAG = "WEATHER";
RTCConfigData& config = ConfigManager::getConfig();

void printWeatherInfo(const WeatherInfo &weather) {
    ESP_LOGI(TAG, "--- WeatherInfo ---");
    ESP_LOGI(TAG, "City: %s", config.cityName);
    ESP_LOGI(TAG, "Current Temp: %s", weather.temperature.c_str());
    ESP_LOGI(TAG, "Condition: %s", weather.condition.c_str());
    ESP_LOGI(TAG, "Max Temp: %s", weather.tempMax.c_str());
    ESP_LOGI(TAG, "Min Temp: %s", weather.tempMin.c_str());
    ESP_LOGI(TAG, "Sunrise: %s", weather.sunrise.c_str());
    ESP_LOGI(TAG, "Sunset: %s", weather.sunset.c_str());
    ESP_LOGI(TAG, "Raw JSON: %s", weather.rawJson.c_str());
    ESP_LOGI(TAG, "Forecast count: %d", weather.forecastCount);
    for (int i = 0; i < weather.forecastCount && i < 12; ++i) {
        const auto &hour = weather.forecast[i];
        ESP_LOGI(TAG, "Hour %d | Time: %s | Temp: %s | Rain Chance: %s | Humidity: %s | Wind Speed: %s | Rainfall: %s | Snowfall: %s | Weather Code: %s | Weather Desc: %s",
             i + 1,
             hour.time.c_str(),
             hour.temperature.c_str(),
             hour.rainChance.c_str(),
             hour.humidity.c_str(),
             hour.windSpeed.c_str(),
             hour.rainfall.c_str(),
             hour.snowfall.c_str(),
             hour.weatherCode.c_str(),
             hour.weatherDesc.c_str());
    }
    ESP_LOGI(TAG, "--- End WeatherInfo ---");
}

void displayWeatherInfo(const WeatherInfo &weather) {
    ESP_LOGI(TAG, "Displaying weather info on e-paper display");
    
    display.init(115200);
    // Initialize display for drawing
    display.setRotation(0); // Portrait orientation
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen(GxEPD_WHITE);
        
        int16_t y = 20; // Starting Y position
        int16_t x = 20; // Left margin
        
        // Title
        display.setFont(&FreeMonoBold18pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(x, y);
        display.print("Weather Info");
        y += 40;
        
        // City
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(x, y);
        display.print("City: ");
        display.print(config.cityName);
        y += 30;
        
        // Current temperature
        display.setCursor(x, y);
        display.print("Temperature: ");
        display.print(weather.temperature.c_str());
        y += 30;
        
        // Condition
        display.setCursor(x, y);
        display.print("Condition: ");
        display.print(weather.condition.c_str());
        y += 30;
        
        // Max/Min temperatures
        display.setCursor(x, y);
        display.print("Max: ");
        display.print(weather.tempMax.c_str());
        display.print("  Min: ");
        display.print(weather.tempMin.c_str());
        y += 30;
        
        // Sunrise/Sunset
        display.setCursor(x, y);
        display.print("Sunrise: ");
        display.print(weather.sunrise.c_str());
        y += 25;
        
        display.setCursor(x, y);
        display.print("Sunset: ");
        display.print(weather.sunset.c_str());
        y += 40;
        
        // Forecast header
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(x, y);
        display.print("Forecast (next 4 hours):");
        y += 30;
        
        // Show first 4 forecast entries
        display.setFont(&FreeMonoBold9pt7b);
        for (int i = 0; i < weather.forecastCount ; ++i) {
            const auto &hour = weather.forecast[i];
            
            display.setCursor(x, y);
            display.print(hour.time.c_str());
            display.print(" ");
            display.print(hour.temperature.c_str());
            display.print(" Rain ");
            display.print(hour.rainChance.c_str());
            y += 22;
            
            // Check if we're running out of vertical space
            if (y > display.height()) {
                ESP_LOGI(TAG, "Display height exceeded, stopping forecast display : Display height: %d, Current Y: %d", display.height(), y);
                break;
            }
        }
        
    } while (display.nextPage());
    
    display.hibernate(); // Put display into low power mode after drawing
    ESP_LOGI(TAG, "Weather info displayed on e-paper");
}
