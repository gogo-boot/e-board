#include "weather_print.h"
#include <Arduino.h>

void printWeatherInfo(const WeatherInfo &weather) {
    Serial.println("--- WeatherInfo ---");
    Serial.printf("City: %s\n", weather.city.c_str());
    Serial.printf("Current Temp: %s\n", weather.temperature.c_str());
    Serial.printf("Condition: %s\n", weather.condition.c_str());
    Serial.printf("Max Temp: %s\n", weather.tempMax.c_str());
    Serial.printf("Min Temp: %s\n", weather.tempMin.c_str());
    Serial.printf("Sunrise: %s\n", weather.sunrise.c_str());
    Serial.printf("Sunset: %s\n", weather.sunset.c_str());
    Serial.printf("Raw JSON: %s\n", weather.rawJson.c_str());
    Serial.printf("Forecast count: %d\n", weather.forecastCount);
    for (int i = 0; i < weather.forecastCount && i < 12; ++i) {
        const auto &hour = weather.forecast[i];
        Serial.printf("-- Hour %d --\n", i + 1);
        Serial.printf("Time: %s\n", hour.time.c_str());
        Serial.printf("Temp: %s\n", hour.temperature.c_str());
        Serial.printf("Rain Chance: %s\n", hour.rainChance.c_str());
        Serial.printf("Humidity: %s\n", hour.humidity.c_str());
        Serial.printf("Wind Speed: %s\n", hour.windSpeed.c_str());
        Serial.printf("Rainfall: %s\n", hour.rainfall.c_str());
        Serial.printf("Snowfall: %s\n", hour.snowfall.c_str());
        Serial.printf("Weather Code: %s\n", hour.weatherCode.c_str());
        Serial.printf("Weather Desc: %s\n", hour.weatherDesc.c_str());
    }
    Serial.println("--- End WeatherInfo ---");
}
