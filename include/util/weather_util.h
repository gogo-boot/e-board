#pragma once
#include <Arduino.h>
#include <icons.h>

class WeatherUtil {
public:
    static icon_name getWeatherIcon(const String& weatherCode);
    static String degreeToCompass(float degree);
    static String uvIndexToGrade(const String& uvIndexStr);
    static String sunshineSecondsToHHMM(const String& secondsStr);
    static String formatWindText(const String& windSpeed, const String& windGust);
    static String formatDateText(const String& isoTime);
    static String getCurrentDateString();
    static String getDayOfWeekFromDateString(const String& dateStr, int format = 0);
};

