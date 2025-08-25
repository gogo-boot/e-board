#pragma once
#include <Arduino.h>

struct WeatherHoulyForecast {
    String time;
    String temperature;
    String weatherCode;
    String rainChance;
    String rainfall;
    String humidity; // Add this new field
};

struct WeatherDailyForecast {
    String time;
    String weatherCode;
    String sunrise;
    String sunset;
    String tempMax;
    String tempMin;
    String uvIndex;
    String precipitation;
    String precipitationSum;
    String precipitationHours;
    String sunshineDuration;
    String apparentTempMin;
    String apparentTempMax;
    String windSpeedMax;
    String windGustsMax;
};

struct WeatherInfo {
    // Current weather
    String time;
    String temperature;
    String precipitation;
    String weatherCode;

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
