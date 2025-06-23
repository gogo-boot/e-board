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
        ESP_LOGI(TAG, "-- Hour %d --", i + 1);
        ESP_LOGI(TAG, "Time: %s", hour.time.c_str());
        ESP_LOGI(TAG, "Temp: %s", hour.temperature.c_str());
        ESP_LOGI(TAG, "Rain Chance: %s", hour.rainChance.c_str());
        ESP_LOGI(TAG, "Humidity: %s", hour.humidity.c_str());
        ESP_LOGI(TAG, "Wind Speed: %s", hour.windSpeed.c_str());
        ESP_LOGI(TAG, "Rainfall: %s", hour.rainfall.c_str());
        ESP_LOGI(TAG, "Snowfall: %s", hour.snowfall.c_str());
        ESP_LOGI(TAG, "Weather Code: %s", hour.weatherCode.c_str());
        ESP_LOGI(TAG, "Weather Desc: %s", hour.weatherDesc.c_str());
    }
    ESP_LOGI(TAG, "--- End WeatherInfo ---");
}
