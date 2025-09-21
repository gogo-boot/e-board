#pragma once
#include <vector>
#include <Arduino.h>
#include <ArduinoJson.h>

struct Station {
    String id;
    String name;
    String type;
};

struct DepartureInfo {
    String line;
    String direction;
    String directionFlag;
    String time;
    String rtTime;
    bool cancelled;
    String track;
    String category;
    String lead; // Service disruption lead text
    String text; // Service disruption full text
};

struct DepartureData {
    String stopId;
    String stopName;
    std::vector<DepartureInfo> departures;
    int departureCount;
};

extern std::vector<Station> stations;

void getNearbyStops(float lat, float lon);
bool getDepartureFromRMV(const char* stopId, DepartureData& departData);
bool populateDepartureData(const DynamicJsonDocument& doc, DepartureData& departData);
