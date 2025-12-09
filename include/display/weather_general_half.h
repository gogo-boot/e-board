#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "api/dwd_weather_api.h"

class WeatherHalfDisplay {
public:
    // Main weather drawing functions
    static void drawHalfScreenWeatherLayout(const WeatherInfo& weather,
                                            int16_t leftMargin, int16_t rightMargin,
                                            int16_t y, int16_t h);

    static void drawWeatherFooter(int16_t x, int16_t y, int16_t h);

private:
    // Weather-specific column drawing functions
    static void drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t dayWeatherInfoY, const WeatherInfo& weather);
    static void drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo& weather);
    static void drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo& weather);

    // Display references
    static int16_t screenWidth;
    static int16_t screenHeight;
};
