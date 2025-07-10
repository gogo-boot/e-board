#pragma once
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"

// Display orientations
enum class DisplayOrientation {
    PORTRAIT = 0,   // 0° rotation
    LANDSCAPE = 1   // 90° rotation
};

// Display modes
enum class DisplayMode {
    HALF_AND_HALF,     // Split screen: weather + departures
    WEATHER_ONLY,      // Full screen weather
    DEPARTURES_ONLY    // Full screen departures
};

// Display regions for partial updates
enum class DisplayRegion {
    FULL_SCREEN,
    LEFT_HALF,     // or TOP_HALF in landscape
    RIGHT_HALF,    // or BOTTOM_HALF in landscape
    WEATHER_AREA,
    DEPARTURE_AREA
};

class DisplayManager {
public:
    // Initialize display with orientation
    static void init(DisplayOrientation orientation = DisplayOrientation::PORTRAIT);
    
    // Set current display mode and orientation
    static void setMode(DisplayMode mode, DisplayOrientation orientation = DisplayOrientation::PORTRAIT);
    
    // Half and half mode - can update one or both halves
    static void displayHalfAndHalf(const WeatherInfo* weather = nullptr, 
                                   const DepartureData* departures = nullptr);
    
    // Full screen modes
    static void displayWeatherOnly(const WeatherInfo& weather);
    static void displayDeparturesOnly(const DepartureData& departures);
    
    // Partial update functions
    static void updateWeatherHalf(const WeatherInfo& weather);
    static void updateDepartureHalf(const DepartureData& departures);
    
    // Utility functions
    static void clearRegion(DisplayRegion region);
    static void drawDivider();
    static void hibernate();
    
private:
    static DisplayMode currentMode;
    static DisplayOrientation currentOrientation;
    static int16_t screenWidth;
    static int16_t screenHeight;
    static int16_t halfWidth;
    static int16_t halfHeight;
    
    // Internal drawing functions
    static void drawWeatherSection(const WeatherInfo& weather, int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawDepartureSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w, int16_t h);
    
    // Coordinate calculation helpers
    static void calculateDimensions();
    static void getRegionBounds(DisplayRegion region, int16_t& x, int16_t& y, int16_t& w, int16_t& h);
    
    // Font and layout helpers
    static void setLargeFont();
    static void setMediumFont();
    static void setSmallFont();
};
