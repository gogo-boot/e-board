#pragma once
#include <vector>
#include <Arduino.h>

struct Station {
    String id;
    String name;
    String type;
};

extern std::vector<Station> stations;

void getNearbyStops();
void getDepartureBoard(const char* stopId);
