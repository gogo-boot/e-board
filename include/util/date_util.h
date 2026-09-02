#pragma once
#include <Arduino.h>

class DateUtil {
public:
    // Return current date in "DD.MM.YYYY Wochentag" format with German day names
    static String getCurrentDateString();
    // Format an ISO date string ("YYYY-MM-DD" or "YYYY-MM-DDTHH:MM") in the same
    // "DD.MM.YYYY Wochentag" format as getCurrentDateString(), for an arbitrary date.
    static String formatFullDateString(const String& isoDate);
};

