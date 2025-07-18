#pragma once
#include "api/dwd_weather_api.h"
#include <Arduino.h>

class WeatherGraph {
public:
    /**
     * Draw a combined temperature line and rain bar chart
     * @param weather Weather data with hourly forecasts
     * @param x X position of graph area
     * @param y Y position of graph area  
     * @param w Width of graph area (should be ~380px for weather section)
     * @param h Height of graph area (333px as specified)
     */
    static void drawTemperatureAndRainGraph(const WeatherInfo& weather, 
                                          int16_t x, int16_t y, 
                                          int16_t w, int16_t h);
    
private:
    // Core drawing functions
    static void drawGraphFrame(int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawTemperatureAxis(int16_t x, int16_t y, int16_t w, int16_t h, 
                                   float minTemp, float maxTemp);
    static void drawRainAxis(int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawTimeAxis(int16_t x, int16_t y, int16_t w, int16_t h,const WeatherInfo& weather);
    static void drawTemperatureLine(const WeatherInfo& weather, 
                                  int16_t graphX, int16_t graphY, 
                                  int16_t graphW, int16_t graphH,
                                  float minTemp, float maxTemp);
    static void drawRainBars(const WeatherInfo& weather,
                           int16_t graphX, int16_t graphY, 
                           int16_t graphW, int16_t graphH);
    
    // Humidity drawing functions
    static void drawHumidityLine(const WeatherInfo& weather,
                               int16_t graphX, int16_t graphY, 
                               int16_t graphW, int16_t graphH);
    static void drawDottedLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    
    // Utility functions
    static float parseTemperature(const String& tempStr);
    static int parseRainChance(const String& rainStr);
    static float parseHumidity(const String& humidityStr);
    static float calculateDynamicMinTemp(float actualMin);
    static float calculateDynamicMaxTemp(float actualMax);
    static int16_t mapToPixel(float value, float minVal, float maxVal, 
                            int16_t minPixel, int16_t maxPixel);
    static String formatHourFromTime(const String& timeStr);
    
    // Constants
    static const int16_t MARGIN_LEFT = 35;    // Space for temperature labels
    static const int16_t MARGIN_RIGHT = 35;   // Space for rain percentage labels
    static const int16_t MARGIN_TOP = 15;     // Top spacing
    static const int16_t MARGIN_BOTTOM = 25;  // Space for time labels
    static const int16_t LEGEND_MARGIN = 35;  // Space for legend labels
    static const int HOURS_TO_SHOW = 13;      // 13 hours as specified
};
