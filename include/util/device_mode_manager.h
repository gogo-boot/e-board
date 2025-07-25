#pragma once
#include <Arduino.h>

class DeviceModeManager {
public:
    static bool hasValidConfiguration(bool &hasValidConfig);
    static void runConfigurationMode();
    static void runOperationalMode();
};
