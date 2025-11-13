#pragma once
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"
#include "config/config_manager.h"

// Display constants - centralized configuration
namespace DisplayConstants {
    constexpr uint32_t SERIAL_BAUD_RATE = 115200;
    constexpr uint16_t RESET_DURATION_MS = 10;
    constexpr int16_t FOOTER_HEIGHT = 15;
    constexpr int16_t MARGIN_HORIZONTAL = 10;
}

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

// Update regions - what parts of the display need updating
enum class UpdateRegion {
    NONE = 0, // No data to display
    WEATHER_ONLY = 1, // Only weather needs update
    DEPARTURE_ONLY = 2, // Only departure needs update
    BOTH = 3 // Both weather and departure need update
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

    // === Configuration Mode Display ===
    // Display setup instructions for configuration phases (in German)
    static void displayPhase1WifiSetup(); // Phase 1: WiFi configuration
    static void displayPhase2AppSetup(); // Phase 2: Application configuration

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
    // Initialization modes
    enum class InitMode {
        LEGACY, // Legacy init for backward compatibility
        FULL_REFRESH, // Clear screen and full refresh
        PARTIAL_UPDATE // Preserve screen content for partial updates
    };

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
    static void initInternal(DisplayOrientation orientation, InitMode mode);
    static UpdateRegion determineUpdateRegion(const WeatherInfo* weather,
                                              const DepartureData* departures);

    // Display update methods for each case
    static void displayBothHalves(const WeatherInfo& weather, const DepartureData& departures);
    static void displayWeatherHalfOnly(const WeatherInfo& weather);
    static void displayDepartureHalfOnly(const DepartureData& departures);
    static void updateWeatherHalf(bool isFullUpdate, const WeatherInfo& weather);
    static void updateDepartureHalf(bool isFullUpdate, const DepartureData& departures);
    static void displayVerticalLine(const int16_t contentY);
    static void calculateDimensions();

    static String shortenTextToFit(const String& text, int16_t maxWidth);
};
