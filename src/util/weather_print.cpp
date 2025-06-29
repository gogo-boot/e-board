#include "weather_print.h"
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "WEATHER";

void printWeatherInfo(const WeatherInfo &weather) {
    ESP_LOGI(TAG, "--- WeatherInfo ---");
    ESP_LOGI(TAG, "City: %s", weather.city.c_str());
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
