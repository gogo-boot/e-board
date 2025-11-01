#pragma once
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"
#include "config/config_manager.h"

// Display orientations
enum class DisplayOrientation {
    LANDSCAPE = 0 // Default: 0° rotation (800x480)
};

// Display modes
enum class DisplayMode {
    HALF_AND_HALF, // Split screen: weather + departures
    WEATHER_ONLY, // Full screen weather
    DEPARTURES_ONLY // Full screen departures
};

// Display regions for partial updates
enum class DisplayRegion {
    FULL_SCREEN,
    // Landscape mode regions
    LEFT_HALF, // Left half in landscape (weather area)
    RIGHT_HALF, // Right half in landscape (departure area)
    // Generic semantic regions
    WEATHER_AREA, // Maps to LEFT_HALF in landscape
    DEPARTURE_AREA // Maps to RIGHT_HALF in landscape
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
    static void displayWeatherFull(const WeatherInfo& weather);
    static void displayDeparturesFull(const DepartureData& departures);

    // Partial update functions
    static void updateWeatherHalf(bool isFullUpate, const WeatherInfo& weather);
    static void updateDepartureHalf(bool isFullUpate, const DepartureData& departures);
    static void displayVerticalLine(const int16_t contentY);

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

    static String getStopName(RTCConfigData& config);

    // Coordinate calculation helpers
    static void calculateDimensions();

    static String shortenTextToFit(const String& text, int16_t maxWidth);
};
