#pragma once
#include <Arduino.h>
#include <vector>

struct MyStationConfig {
    float latitude = 0.0;
    float longitude = 0.0;
    String ssid; // changed from routerName to ssid
    String ipAddress;
    String cityName;
    std::vector<String> oepnvFilters; // e.g. {"RE", "S-Bahn", "Bus"}
    std::vector<String> stopIds;   // all found stop IDs
    std::vector<String> stopNames; // all found stop names
    String selectedStopId;   // User's selected stop ID from config
    String selectedStopName; // User's selected stop name from config
};

extern MyStationConfig g_stationConfig;
