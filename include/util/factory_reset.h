#pragma once
#include <Arduino.h>
#include <nvs_flash.h>
// Hold duration in milliseconds to trigger factory reset
const unsigned long HOLD_DURATION_MS = 3000;

// Reset button GPIO
const gpio_num_t RESET_BUTTON_GPIO = GPIO_NUM_2;

class FactoryReset {
public:
    static boolean checkFactoryResetButton();

    static void performFactoryReset();
};
