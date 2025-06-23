#pragma once
#include <Arduino.h>

struct WeatherInfo {
    String temperature;
    String condition;
    String city;
    String rawJson;
};

bool getWeatherFromDWD(float lat, float lon, WeatherInfo &weather);
