#pragma once
#include <Arduino.h>

class Util {
public:
    static void printFreeHeap(const char* msg);
    static String urlEncode(const String& str);
    static String getUniqueSSID(const String& prefix);
    static String shortenDestination(String departure, String destination);
    static String urlDecode(const String& str);
};
