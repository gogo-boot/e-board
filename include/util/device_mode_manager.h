#pragma once
#include "display/display_manager.h" // Include for DisplayMode and DisplayOrientation
#include "config/config_struct.h" // Include for ConfigPhase enum

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

    // Configuration phase management
    static ConfigPhase getCurrentPhase();
    static void showPhaseInstructions(ConfigPhase phase);
    static void showWifiErrorPage();

    // Common operational mode functions
    static bool setupOperationalMode();
    static bool setupConnectivityAndTime();
    static void enterOperationalSleep();

    // Helper functions for data fetching
    static bool fetchWeatherData(WeatherInfo& weather);
    static bool fetchTransportData(DepartureData& depart);
};
