#pragma once
#include <Arduino.h>

class DeviceModeManager {
public:
    static bool hasValidConfiguration();
    static void runConfigurationMode();
    static void runOperationalMode();
};
