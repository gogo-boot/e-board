#pragma once
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"
#include "config/config_manager.h"

// Display orientations
enum class DisplayOrientation
{
    LANDSCAPE = 0 // Default: 0° rotation (800x480)
};

// Display modes
enum class DisplayMode
{
    HALF_AND_HALF,  // Split screen: weather + departures
    WEATHER_ONLY,   // Full screen weather
    DEPARTURES_ONLY // Full screen departures
};

// Display regions for partial updates
enum class DisplayRegion
{
    FULL_SCREEN,
    // Landscape mode regions
    LEFT_HALF,  // Left half in landscape (weather area)
    RIGHT_HALF, // Right half in landscape (departure area)
    // Generic semantic regions
    WEATHER_AREA,  // Maps to LEFT_HALF in landscape
    DEPARTURE_AREA // Maps to RIGHT_HALF in landscape
};

class DisplayManager
{
public:
    // Initialize display with orientation (default: landscape)
    static void init(DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);

    // Set current display mode and orientation (default: landscape)
    static void setMode(DisplayMode mode, DisplayOrientation orientation = DisplayOrientation::LANDSCAPE);

    // Half and half mode - can update one or both halves
    static void displayHalfAndHalf(const WeatherInfo *weather = nullptr,
                                   const DepartureData *departures = nullptr);

    // Full screen modes
    static void displayWeatherOnly(const WeatherInfo &weather);
    static void displayDeparturesOnly(const DepartureData &departures);

    // Partial update functions
    static void updateWeatherHalf(bool isFullUpate, const WeatherInfo &weather);
    static void updateDepartureHalf(bool isFullUpate, const DepartureData &departures);
    static void displayVerticalLine(bool isFullUpate, const int16_t contentY);

    // Utility functions
    static void clearRegion(DisplayRegion region);
    static void drawDivider();
    static void hibernate();

    // Font helpers (public for use by graph components)
    static void setSmallFont();
    static int16_t getTextWidth(const String &text);

private:
    static DisplayMode currentMode;
    static DisplayOrientation currentOrientation;
    static int16_t screenWidth;
    static int16_t screenHeight;
    static int16_t halfWidth;
    static int16_t halfHeight;

    // Internal drawing functions
    static void drawWeatherSection(const WeatherInfo &weather, int16_t x, int16_t y, int16_t w, int16_t h);
    static void weatherInfoThirdColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather);
    static void weatherInfoSecondColumn(int16_t currentX, int16_t dayWeatherInfoY, const WeatherInfo &weather);
    static void wertherInfoFirstColumn(int16_t leftMargin, int16_t &currentY, const WeatherInfo &weather);
    static void drawWeatherFooter(int16_t x, int16_t y);
    static void drawDepartureSection(const DepartureData &departures, int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawHeaderSection(bool isFullUpate, int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawDepartureFooter(int16_t x, int16_t y);

    static String getStopName(RTCConfigData &config);

    // Coordinate calculation helpers
    static void calculateDimensions();
    static void getRegionBounds(DisplayRegion region, int16_t &x, int16_t &y, int16_t &w, int16_t &h);

    // Font and layout helpers
    static void setLargeFont();
    static void setMediumFont();
    // setSmallFont moved to public section

    // Departure-specific larger fonts
    static void setDepartureLargeFont();
    static void setDepartureMediumFont();
    static void setDepartureSmallFont();

    // Text width measurement helpers
    // getTextWidth moved to public section
    static int16_t getTextExcess(const String &text, int16_t maxWidth);
    static String shortenTextToFit(const String &text, int16_t maxWidth);

    // Text wrapping helper
    static void printWrappedText(const String &text, int16_t x, int16_t &y,
                                 int16_t maxWidth, int16_t maxChars, int16_t lineHeight);

};
