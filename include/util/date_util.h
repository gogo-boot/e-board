#pragma once
#include <Arduino.h>

class DateUtil {
public:
    // Format ISO date string (e.g., "2025-07-16T15:30") to "DD. Monat" format with German month names
    static String formatDateText(const String& isoTime);
    // Return current date in "DD.MM.YYYY Wochentag" format with German day names
    static String getCurrentDateString();
    // Converts date string (YYYY-MM-DD or YYYY-MM-DDTHH:MM) to German day name (full, 2-char, or 3-char)
    static String getDayOfWeekFromDateString(const String& dateStr, int format = 0);
};

