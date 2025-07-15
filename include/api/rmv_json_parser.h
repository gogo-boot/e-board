#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "api/rmv_api.h"

// Maximum number of departures to parse (matches display limitations)
#define MAX_DEPARTURES 20

class RMVStreamParser {
public:
    static bool parseResponse(const String& payload, DepartureData& departData);
    
private:
    static bool findAndParseDepartures(const String& json, DepartureData& departData);
    static String extractJsonValue(const String& json, const String& key, int startPos = 0);
    static int findJsonArrayStart(const String& json, const String& arrayName, int startPos = 0);
    static bool parseIndividualDeparture(const String& departureJson, DepartureInfo& info);
    static void parseMessagesArray(const String& json, DepartureInfo& info);
    static String extractNestedValue(const String& json, const String& path);
};
