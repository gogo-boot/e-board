#pragma once
#include <Arduino.h>

struct WeatherHoulyForecast {
    String time;
    float temperature;
    int weatherCode;
    int rainChance;
    float rainfall;
    int humidity;
};

struct WeatherDailyForecast {
    String time;
    int windDirection;
    int weatherCode;
    String sunrise;
    String sunset;
    float tempMax;
    float tempMin;
    float uvIndex;
    float precipitationSum;
    int precipitationHours;
    float sunshineDuration;
    float apparentTempMin;
    float apparentTempMax;
    float windSpeedMax;
    float windGustsMax;
};

struct WeatherInfo {
    // Current weather
    String time;
    float temperature;
    float precipitation;
    int weatherCode;

    // Hourly forecast
    WeatherHoulyForecast hourlyForecast[13]; // 1hour past and 12-hour forecast
    int hourlyForecastCount = 0;

    // Daily forecast
    WeatherDailyForecast dailyForecast[7]; // 14-day forecast
    int dailyForecastCount = 0;
};

bool getGeneralWeatherHalf(float lat, float lon, WeatherInfo& weather);
bool getGeneralWeatherFull(float lat, float lon, WeatherInfo& weather);
String getCityFromLatLon(float lat, float lon);
