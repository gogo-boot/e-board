#pragma once
#include <Arduino.h>
#include <cstdint>

// Print wakeup reason after deep sleep
void printWakeupReason();

// Enhanced deep sleep function with multiple wakeup options
void enterDeepSleep(uint64_t sleepTimeUs);
