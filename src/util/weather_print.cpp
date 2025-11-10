#include "util/weather_print.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"

static const char* TAG = "WEATHER";

void printWeatherInfo(const WeatherInfo& weather) {
    RTCConfigData& config = ConfigManager::getConfig();
    ESP_LOGI(TAG, "--- WeatherInfo ---");
    ESP_LOGI(TAG, "City: %s", config.cityName);
    ESP_LOGI(TAG, "Current: %.1f°C, %.1f mm, Weather Code: %d", weather.temperature,
             weather.precipitation, weather.weatherCode);

    ESP_LOGI(TAG, "Hourly forecast count: %d", weather.hourlyForecastCount);
    if (weather.hourlyForecastCount > 0) {
        for (int i = 0; i < weather.hourlyForecastCount; i++) {
            const auto& hour = weather.hourlyForecast[i];
            ESP_LOGI(TAG, "Hour %d: %s | %.1f°C | Code: %d | Rain: %d%% (%.2f mm) | Humidity: %d%%",
                     i + 1,
                     hour.time.c_str(),
                     hour.temperature,
                     hour.weatherCode,
                     hour.rainChance,
                     hour.rainfall,
                     hour.humidity);
        }
    } else {
        ESP_LOGI(TAG, "No hourly forecast data available");
    }

    // Daily forecast information (if available)
    if (weather.dailyForecastCount > 0) {
        ESP_LOGI(TAG, "Daily Forecast count: %d", weather.dailyForecastCount);
        for (int i = 0; i < weather.dailyForecastCount && i < 7; ++i) {
            const auto& day = weather.dailyForecast[i];
            ESP_LOGI(
                TAG,
                "Day %d: %s | Code: %d | Sun: %s-%s | Temp: %.1f°-%.1f°C | UV: %.1f | Apparent: %.1f°-%.1f°C | Sunshine: %.0fs | Rain: %.1fmm, %dh | Wind: %d° %.1fkm/h (gust %.1fkm/h)",
                i + 1, day.time.c_str(), day.weatherCode,
                day.sunrise.c_str(), day.sunset.c_str(),
                day.tempMin, day.tempMax, day.uvIndex,
                day.apparentTempMin, day.apparentTempMax,
                day.sunshineDuration,
                day.precipitationSum,
                day.precipitationHours,
                day.windDirection,
                day.windSpeedMax,
                day.windGustsMax);
        }
    } else {
        ESP_LOGI(TAG, "No daily forecast data available");
    }

    ESP_LOGI(TAG, "--- End WeatherInfo ---");
}
