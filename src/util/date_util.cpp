#include "util/date_util.h"
#include "util/time_manager.h"
#include <time.h>

// Shared "DD.MM.YYYY Wochentag" formatter so the current-date and arbitrary-date
// paths produce byte-identical output and can never drift.
static String formatFullDate(const tm& timeinfo) {
    static const char* dayNames[] = {
        "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
    };
    int wday = timeinfo.tm_wday;
    if (wday < 0 || wday > 6) wday = 0;
    char dateStr[40];
    snprintf(dateStr, sizeof(dateStr), "%02d.%02d.%04d %s",
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, dayNames[wday]);
    return String(dateStr);
}

String DateUtil::formatDateText(const String& isoTime) {
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

String DateUtil::getCurrentDateString() {
    if (TimeManager::isTimeSet()) {
        tm timeinfo;
        if (TimeManager::getCurrentLocalTime(timeinfo)) {
            return formatFullDate(timeinfo);
        }
    }
    return "";
}

String DateUtil::formatFullDateString(const String& isoDate) {
    if (isoDate.length() < 10) {
        return "";
    }
    tm timeinfo = {};
    timeinfo.tm_year = isoDate.substring(0, 4).toInt() - 1900;
    timeinfo.tm_mon  = isoDate.substring(5, 7).toInt() - 1;
    timeinfo.tm_mday = isoDate.substring(8, 10).toInt();
    mktime(&timeinfo);  // normalizes and fills tm_wday
    return formatFullDate(timeinfo);
}

String DateUtil::getDayOfWeekFromDateString(const String& dateStr, int format) {
    int year = 0, month = 0, day = 0;
    if (dateStr.length() >= 10) {
        year = dateStr.substring(0, 4).toInt();
        month = dateStr.substring(5, 7).toInt();
        day = dateStr.substring(8, 10).toInt();
    } else {
        return "";
    }
    tm timeinfo = {};
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

