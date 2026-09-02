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

// =============================================================================
// Day browsing RTC cache
// =============================================================================
// Compact hourly point stored in RTC memory for instant day-browse rendering.
// The time string is dropped: the array index within a day maps directly to the
// hour (index 0 = 00:00, index 6 = 06:00, ...), so no per-point timestamp needed.
struct DayBrowsePoint {
    float   temperature;   // 4 bytes (°C)
    float   rainfall;      // 4 bytes (mm)
    int16_t rainChance;    // 2 bytes (%)
    uint8_t weatherCode;   // 1 byte
    uint8_t humidity;      // 1 byte (%)
};                         // 12 bytes

static constexpr int DAY_CACHE_MAX_DAYS = 7;   // days 0..6 (index 0 = today)
static constexpr int DAY_CACHE_HOURS    = 24;  // 00:00–23:00 per day

// Day-browse display window: 06:00 through 24:00 inclusive (19 points).
// The 24:00 point is the next day's 00:00, giving 18 intervals that divide
// cleanly by 3 for aligned 3-hour grid lines. The last browsable day (with no
// next-day data cached) falls back to 18 points (06:00–23:00).
static constexpr int DAY_BROWSE_START_HOUR = 6;
static constexpr int DAY_BROWSE_HOURLY_COUNT = DAY_CACHE_HOURS - DAY_BROWSE_START_HOUR + 1; // 19 (max)

// Fetch full hourly data (00:00–23:00) for up to maxDays days in a SINGLE
// Open-Meteo call and populate the RTC cache. Limited-day models return null
// temperatures past their range; such hours are skipped. A day is only counted
// as valid if hours 06:00–23:00 are all present (non-null).
//
// Returns the number of fully-valid days written (>= 1). On HTTP/JSON failure
// returns 0 and leaves the cache untouched.
int getWeatherHourlyMultiDay(float lat, float lon, int maxDays,
                             DayBrowsePoint cache[][DAY_CACHE_HOURS]);
