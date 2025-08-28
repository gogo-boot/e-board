#pragma once
#include <Arduino.h>
#include <icons.h>

class Util {
public:
    static void printFreeHeap(const char* msg);
    static String urlEncode(const String& str);
    static String getUniqueSSID(const String& prefix);
    static String shortenDestination(String departure, String destination);
    static String urlDecode(const String& str);
    static icon_name getWeatherIcon(const String& weatherCode);
    static String degreeToCompass(float degree);
    static String uvIndexToGrade(const String& uvIndexStr);
    static String sunshineSecondsToHHMM(const String& secondsStr);
    static String getDateString(const String& dateStr);
    static String formatWindText(const String& windSpeed, const String& windGust);
    static String formatDateText(const String& isoTime);
    static String getCurrentDateString();
};
