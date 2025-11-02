#pragma once
#include <Arduino.h>
#include "display/display_manager.h" // Include for DisplayMode and DisplayOrientation

// Forward declarations for data structures
struct WeatherInfo;
struct DepartureData;

class DeviceModeManager {
public:
    static bool hasValidConfiguration(bool& hasValidConfig);
    static void runConfigurationMode();
    static void showWeatherDeparture();
    static void updateWeatherFull();
    static void updateDepartureFull();

    // Common operational mode functions
    static bool setupOperationalMode();
    static bool setupConnectivityAndTime();
    static void enterOperationalSleep();

    // Helper functions for data fetching
    static bool fetchWeatherData(WeatherInfo& weather);
    static bool fetchTransportData(DepartureData& depart);
};
