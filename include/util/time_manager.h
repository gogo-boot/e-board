#pragma once
#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
    static void setupNTPTime();
    static void printCurrentTime();
    static bool isTimeSet();
};
