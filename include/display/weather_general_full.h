#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "api/dwd_weather_api.h"

class WeatherFullDisplay {
public:
    // Weather layout helpers
    static void drawFullScreenWeatherLayout(const WeatherInfo& weather);

    static void drawWeatherFooter(int16_t x, int16_t y, int16_t h);

private:
    // Display references
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* display;
    static U8G2_FOR_ADAFRUIT_GFX* u8g2;
    static int16_t screenWidth;
    static int16_t screenHeight;
};
