#pragma once
#include <Arduino.h>
#define TIME_STRING_LENGTH 17      // "2025-08-25T22:00" + null terminator
#define TIME_SHORT_LENGTH 6        // "22:00" + null terminator

struct WeatherHourlyForecast {
    char time[TIME_STRING_LENGTH];
    float temperature;
    int weatherCode;
    int rainChance;
    float rainfall;
    int humidity;
};

struct WeatherDailyForecast {
    char time[TIME_STRING_LENGTH];
    int windDirection;
    int weatherCode;
    char sunrise[TIME_STRING_LENGTH];
    char sunset[TIME_STRING_LENGTH];
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
    char time[TIME_STRING_LENGTH];
    float temperature;
    float precipitation;
    int weatherCode;

    // Hourly forecast
    WeatherHourlyForecast hourlyForecast[13]; // 1hour past and 12-hour forecast
    int hourlyForecastCount;

    // Daily forecast
    WeatherDailyForecast dailyForecast[7]; // 7-day forecast
    int dailyForecastCount;
};

bool getGeneralWeatherFull(float lat, float lon, WeatherInfo& weather);
String getCityFromLatLon(float lat, float lon);
void safeStringCopy(char* dest, const String& src, size_t destSize);
void extractTimeFromISO(char* dest, const String& isoDateTime, size_t destSize);

// Day browsing: fetch 19h hourly data for a specific future day (06:00-00:00)
static constexpr int DAY_BROWSE_HOURLY_COUNT = 19;
bool getWeatherForDay(float lat, float lon, int dayOffset,
                      WeatherHourlyForecast hourlyOut[], int& hourlyCount);
