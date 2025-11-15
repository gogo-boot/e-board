#pragma once
#include <Arduino.h>
#include <nvs_flash.h>
#include "config/pins.h"

// Hold duration in milliseconds to trigger factory reset
const unsigned long HOLD_DURATION_MS = 3000;


class FactoryReset {
public:
    static boolean checkFactoryResetButton();

    static void performFactoryReset();
};
