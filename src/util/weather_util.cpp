#include "util/weather_util.h"
#include "util/time_manager.h"
#include <icons.h>
#include <time.h>

icon_name WeatherUtil::getWeatherIcon(const String& weatherCode) {
    int code = weatherCode.toInt();
    switch (code) {
    case 0: return wi_0_day_sunny;
    case 1:
    case 2:
    case 3: return wi_1_day_sunny_overcast;
    case 45:
    case 48: return wi_45_day_fog;
    case 51:
    case 53:
    case 55: return wi_51_rain_mix;
    case 56:
    case 57: return wi_56_rain_mix;
    case 61:
    case 63:
    case 65: return wi_61_rain;
    case 66:
    case 67: return wi_66_rain_mix;
    case 71:
    case 73:
    case 75: return wi_71_snow_wind;
    case 77: return wi_77_day_snow_wind;
    case 80:
    case 81:
    case 82: return wi_81_showers;
    case 85:
    case 86: return wi_85_snow_wind;
    case 95: return wi_95_thunderstorm;
    case 96:
    case 99: return wi_99_thunderstorm;
    default: return wi_0_day_sunny;
    }
}

String WeatherUtil::degreeToCompass(float degree) {
    static const char* directions[] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    int idx = (int)((degree + 11.25) / 22.5);
    idx = idx % 16;
    return String(directions[idx]);
}

String WeatherUtil::uvIndexToGrade(const String& uvIndexStr) {
    float uv = uvIndexStr.toFloat();
    String grade;
    if (uv < 3) grade = "Low";
    else if (uv < 6) grade = "Moderate";
    else if (uv < 8) grade = "High";
    else if (uv < 11) grade = "Very High";
    else grade = "Extreme";
    return grade + " (" + uvIndexStr + ")";
}

String WeatherUtil::sunshineSecondsToHHMM(const String& secondsStr) {
    long seconds = secondsStr.toFloat();
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hours, minutes);
    return String(buf);
}

String WeatherUtil::formatWindText(const String& windSpeed, const String& windGust) {
    return "Wind : " + windSpeed + " - " + windGust + " m/s";
}

String WeatherUtil::formatDateText(const String& isoTime) {
    int year = 0, month = 0, day = 0;
    if (isoTime.length() >= 10) {
        year = isoTime.substring(0, 4).toInt();
        month = isoTime.substring(5, 7).toInt();
        day = isoTime.substring(8, 10).toInt();
    }
    static const char* monthNames[] = {
        "", "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November",
        "Dezember"
    };
    if (month > 0 && month <= 12 && day > 0) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%02d. %s", day, monthNames[month]);
        return String(buf);
    } else {
        return "Datum: N/A";
    }
}

String WeatherUtil::getCurrentDateString() {
    if (TimeManager::isTimeSet()) {
        struct tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            static const char* dayNames[] = {
                "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
            };
            char dateStr[40];
            int wday = timeinfo.tm_wday;
            if (wday < 0 || wday > 6) wday = 0;
            snprintf(dateStr, sizeof(dateStr), "%02d.%02d.%04d %s", timeinfo.tm_mday, timeinfo.tm_mon + 1,
                     timeinfo.tm_year + 1900, dayNames[wday]);
            return String(dateStr);
        }
    }
    return "";
}

String WeatherUtil::getDayOfWeekFromDateString(const String& dateStr, int format) {
    int year = 0, month = 0, day = 0;
    if (dateStr.length() >= 10) {
        year = dateStr.substring(0, 4).toInt();
        month = dateStr.substring(5, 7).toInt();
        day = dateStr.substring(8, 10).toInt();
    } else {
        return "";
    }
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    mktime(&timeinfo);
    static const char* dayNamesFull[] = {
        "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
    };
    static const char* dayNames2[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    static const char* dayNames3[] = {"Son", "Mon", "Die", "Mit", "Don", "Fre", "Sam"};
    int wday = timeinfo.tm_wday;
    if (wday < 0 || wday > 6) wday = 0;
    if (format == 2) return String(dayNames2[wday]);
    if (format == 3) return String(dayNames3[wday]);
    return String(dayNamesFull[wday]);
}

