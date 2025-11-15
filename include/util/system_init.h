#pragma once

#include <Arduino.h>

namespace SystemInit {
    void initialize();
    void printWakeupCause();
    bool checkAndHandleFactoryReset();
}
