#pragma once
#include "config/config_struct.h" // Include for ConfigPhase enum

// Forward declarations for data structures
struct WeatherInfo;
struct DepartureData;

class DeviceModeManager {
public:
    static void runConfigurationMode();
    static void showWeatherDeparture();
    static void updateWeatherFull();
    static void updateDepartureFull();

    // Configuration phase management
    static ConfigPhase getCurrentPhase();
    static void showPhaseInstructions(ConfigPhase phase);
    static void showWifiErrorPage();

    // Common operational mode functions
    static bool setupConnectivityAndTime();

    // Helper functions for data fetching
    static bool fetchWeatherData(WeatherInfo& weather);
    static bool fetchTransportData(DepartureData& depart);
};
