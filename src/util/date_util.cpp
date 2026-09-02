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

