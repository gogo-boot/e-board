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
    // === High-Level Refresh Methods (Centralized Control) ===
    // These methods handle initialization + display in one call
    static void refreshFullScreen(const WeatherInfo* weather, const DepartureData* departures);
    static void refreshWeatherHalf(const WeatherInfo* weather);
    static void refreshDepartureHalf(const DepartureData* departures);
    static void refreshWeatherFullScreen(const WeatherInfo& weather);
    static void refreshDepartureFullScreen(const DepartureData& departures);

    // === Legacy Display Methods (for backward compatibility) ===
    static void displayHalfAndHalf(const WeatherInfo* weather = nullptr,
                                   const DepartureData* departures = nullptr);
    static void displayWeatherFull(const WeatherInfo& weather);
    static void displayDeparturesFull(const DepartureData& departures);

    // === Initialization Methods ===
    static void init(DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);
    static void initForFullRefresh(DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);
    static void initForPartialUpdate(DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);

    // Set current display mode and orientation (default: landscape)
    static void setMode(DisplayMode mode, DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);

    // Utility functions
    static void hibernate();
    static bool isInitialized() { return initialized; }

private:
    // Internal state
    static bool initialized;
    static bool partialMode;
    static DisplayMode currentMode;
    static DisplayOrientation currentOrientation;
    static int16_t screenWidth;
    static int16_t screenHeight;
    static int16_t halfWidth;
    static int16_t halfHeight;

    // Internal helper methods
    static void updateWeatherHalf(bool isFullUpdate, const WeatherInfo& weather);
    static void updateDepartureHalf(bool isFullUpdate, const DepartureData& departures);
    static void displayVerticalLine(const int16_t contentY);
    static void calculateDimensions();

    static String shortenTextToFit(const String& text, int16_t maxWidth);
};
