#pragma once
#include <Arduino.h>
#include <cstdint>

// Print wakeup reason after deep sleep
void printWakeupReason();

// Calculate sleep time until next scheduled interval wakeup
uint64_t calculateSleepTime(int wakeupIntervalMinutes = 5);

// Calculate sleep time until specific time (e.g., 01:00)
uint64_t calculateSleepUntilTime(int targetHour, int targetMinute = 0);

// Enhanced deep sleep function with multiple wakeup options
void enterDeepSleep(uint64_t sleepTimeUs);
