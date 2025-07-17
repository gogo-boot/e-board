#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "api/dwd_weather_api.h"
#include "config/config_manager.h"

class WeatherDisplay {
public:
    // Main weather drawing functions
    static void drawWeatherSection(const WeatherInfo &weather, int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawWeatherFooter(int16_t x, int16_t y);
    
    // Initialize with display references and screen dimensions
    static void init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                     U8G2_FOR_ADAFRUIT_GFX &u8g2Ref, 
                     int16_t screenW, int16_t screenH);

private:
    // Weather-specific column drawing functions
    static void drawWeatherInfoFirstColumn(int16_t leftMargin, int16_t dayWeatherInfoY, const WeatherInfo &weather);
    static void drawWeatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather);
    static void drawWeatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather);
    
    // Weather layout helpers
    static void drawFullScreenWeatherLayout(const WeatherInfo &weather, 
                                           int16_t leftMargin, int16_t rightMargin, 
                                           int16_t &currentY, int16_t y, int16_t h);
    static void drawHalfScreenWeatherLayout(const WeatherInfo &weather, 
                                           int16_t leftMargin, int16_t rightMargin, 
                                           int16_t &currentY, int16_t y, int16_t h);
    static void drawCompactTextForecast(const WeatherInfo &weather, 
                                       int16_t leftMargin, int16_t &currentY, 
                                       int16_t y, int16_t h, int availableHeight);
    
    // Display references
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *display;
    static U8G2_FOR_ADAFRUIT_GFX *u8g2;
    static int16_t screenWidth;
    static int16_t screenHeight;
};
