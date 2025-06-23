#pragma once
#include <Arduino.h>

struct WeatherForecast {
    String time;
    String temperature;
    String rainChance;
    String humidity;
    String windSpeed;
    String rainfall;
    String snowfall;
    String weatherCode;
    String weatherDesc;
};

struct WeatherInfo {
    String temperature;
    String condition;
    String city;
    String rawJson;
    WeatherForecast forecast[12]; // 12-hour forecast
    int forecastCount = 0;
    // Daily
    String tempMax;
    String tempMin;
    String sunrise;
    String sunset;
};

bool getWeatherFromDWD(float lat, float lon, WeatherInfo &weather);
