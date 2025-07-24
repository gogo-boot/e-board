#pragma once
#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
    static void setupNTPTime();
    static String getGermanDateTimeString();
    static void printCurrentTime();
    static bool isTimeSet();
    static bool getCurrentLocalTime(struct tm& timeinfo);
};
