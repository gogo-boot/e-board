#pragma once

#include <Arduino.h>
#include "api/dwd_weather_api.h"

class WeatherFullDisplay {
public:
    // Weather layout helpers
    static void drawFullScreenWeatherLayout(const WeatherInfo& weather);

    // Day browse layout: full-width graph for a selected future day
    static void drawDayBrowseLayout(const WeatherInfo& weather,
                                     const WeatherHourlyForecast dayHourly[],
                                     int hourlyCount,
                                     int selectedDay);

    static void drawWeatherFooter(int16_t x, int16_t y, int16_t h);

private:
    // Display references
    static int16_t screenWidth;
    static int16_t screenHeight;
};
