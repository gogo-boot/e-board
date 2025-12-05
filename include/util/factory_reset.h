#pragma once
#include <Arduino.h>

// Hold duration in milliseconds to trigger factory reset
const unsigned long HOLD_DURATION_MS = 3000;


class FactoryReset {
public:
    static boolean checkFactoryResetButton();

    static void performFactoryReset();
};
