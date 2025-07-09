#pragma once
#include <vector>
#include <Arduino.h>

struct Station {
    String id;
    String name;
    String type;
};

struct DepartureInfo {
    String line;
    String direction;
    String time;
    String rtTime;
    String track;
    String category;
};

struct DepartureData {
    String stopId;
    String stopName;
    std::vector<DepartureInfo> departures;
    int departureCount;
};

extern std::vector<Station> stations;

void getNearbyStops(float lat, float lon);
void getDepartureBoard(const char* stopId);
bool getDepartureFromRMV(const char* stopId, DepartureData& departData);
