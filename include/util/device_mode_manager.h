#pragma once
#include <Arduino.h>
#include "display/display_manager.h" // Include for DisplayMode and DisplayOrientation

class DeviceModeManager {
public:
    static bool hasValidConfiguration(bool& hasValidConfig);
    static void runConfigurationMode();
    static void showWeatherDeparture();
    static void showGeneralWeather();
    static void showMarineWeather();
    static void showDeparture();

    // Common operational mode functions for refactoring
    static bool setupOperationalMode();
    static bool setupConnectivityAndTime();
    static void initializeDisplay(DisplayMode mode, DisplayOrientation orientation);
    static void enterOperationalSleep();
};
