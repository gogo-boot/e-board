#pragma once
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"

// Display orientations
enum class DisplayOrientation {
    LANDSCAPE = 0,  // Default: 0° rotation (800x480)
    PORTRAIT = 1    // 90° rotation (480x800)
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
    // Landscape mode regions
    LEFT_HALF,         // Left half in landscape (weather area)
    RIGHT_HALF,        // Right half in landscape (departure area)
    // Portrait mode regions  
    UPPER_HALF,        // Upper half in portrait (weather area)
    LOWER_HALF,        // Lower half in portrait (departure area)
    // Generic semantic regions
    WEATHER_AREA,      // Maps to LEFT_HALF (landscape) or UPPER_HALF (portrait)
    DEPARTURE_AREA     // Maps to RIGHT_HALF (landscape) or LOWER_HALF (portrait)
};

class DisplayManager {
public:
    // Initialize display with orientation (default: landscape)
    static void init(DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);
    
    // Set current display mode and orientation (default: landscape)
    static void setMode(DisplayMode mode, DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);
    
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
    static void drawHeaderSection(int16_t x, int16_t y, int16_t w, int16_t h);
    
    // Coordinate calculation helpers
    static void calculateDimensions();
    static void getRegionBounds(DisplayRegion region, int16_t& x, int16_t& y, int16_t& w, int16_t& h);
    
    // Font and layout helpers
    static void setLargeFont();
    static void setMediumFont();
    static void setSmallFont();
    
    // Departure-specific larger fonts
    static void setDepartureLargeFont();
    static void setDepartureMediumFont();
    static void setDepartureSmallFont();
    
    // Text width measurement helpers
    static int16_t getTextWidth(const String& text);
    static int16_t getTextExcess(const String& text, int16_t maxWidth);
    static String shortenTextToFit(const String& text, int16_t maxWidth);
    
    // Text wrapping helper
    static void printWrappedText(const String& text, int16_t x, int16_t& y, 
                                int16_t maxWidth, int16_t maxChars, int16_t lineHeight);
};
