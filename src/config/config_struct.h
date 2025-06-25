#pragma once
#include <Arduino.h>
#include <vector>

struct MyStationConfig {
    float latitude = 0.0;
    float longitude = 0.0;
    String ssid; // changed from routerName to ssid
    String cityName;
    std::vector<String> oepnvFilters; // e.g. {"RE", "S-Bahn", "Bus"}
    std::vector<String> stopIds;   // all found stop IDs
    std::vector<String> stopNames; // all found stop names
};

extern MyStationConfig g_stationConfig;
