#include "util/weather_print.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"

static const char* TAG = "WEATHER";
RTCConfigData& config = ConfigManager::getConfig();

void printWeatherInfo(const WeatherInfo &weather) {
    ESP_LOGI(TAG, "--- WeatherInfo ---");
    ESP_LOGI(TAG, "City: %s", config.cityName);
    ESP_LOGI(TAG, "Current: %s°C, %s mm, Weather Code: %s", 
             weather.temperature.c_str(), 
             weather.precipitation.c_str(),
             weather.weatherCode.c_str());

    ESP_LOGI(TAG, "Hourly forecast count: %d", weather.hourlyForecastCount);
    if (weather.hourlyForecastCount > 0) {
        for (int i = 0; i < weather.hourlyForecastCount; i++) {
            const auto& hour = weather.hourlyForecast[i];
            ESP_LOGI(TAG, "Hour %d: %s | %s°C | Code: %s | Rain: %s%% (%s mm) | Humidity: %s%%", 
                     i + 1,
                     hour.time.c_str(),
                     hour.temperature.c_str(),
                     hour.weatherCode.c_str(),
                     hour.rainChance.c_str(),
                     hour.rainfall.c_str(),
                     hour.humidity.c_str()); // Add humidity to logs
        }
    } else {
        ESP_LOGI(TAG, "No hourly forecast data available");
    }

    // Daily forecast information (if available)
    if (weather.dailyForecastCount > 0) {
        ESP_LOGI(TAG, "Daily Forecast count: %d", weather.dailyForecastCount);
        for (int i = 0; i < weather.dailyForecastCount && i < 7; ++i) {
            const auto &day = weather.dailyForecast[i];
            ESP_LOGI(TAG, "Day %d | Time: %s | Weather Code: %s | Sunrise: %s | Sunset: %s | Max: %s°C | Min: %s°C | UV Index: %s | Precipitation: %s mm | Sunshine: %s seconds",
                 i + 1,
                 day.time.c_str(),
                 day.weatherCode.c_str(),
                 day.sunrise.c_str(),
                 day.sunset.c_str(),
                 day.tempMax.c_str(),
                 day.tempMin.c_str(),
                 day.uvIndex.c_str(),
                 day.precipitation.c_str(),
                 day.sunshineDuration.c_str());
        }
    } else {
        ESP_LOGI(TAG, "No daily forecast data available");
    }

    ESP_LOGI(TAG, "Raw JSON length: %d characters", weather.rawJson.length());
    ESP_LOGI(TAG, "--- End WeatherInfo ---");
}