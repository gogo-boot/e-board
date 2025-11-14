#pragma once
#include <Arduino.h>
#include <vector>

// Configuration phase tracking
enum ConfigPhase {
    PHASE_WIFI_SETUP = 1, // WiFi credentials needed
    PHASE_APP_SETUP = 2, // WiFi OK, app settings needed
    PHASE_COMPLETE = 3 // All configured
};

// This Struct is only holding values from API ans for showing on configureation web interface
// It is used to hold dynamic data like stopNames, stopIds, and stopDistances from API calls
// The values will be temporarily stored in RAM, after deep sleep they will be removed
struct ConfigOption {
    float latitude = 0.0;
    float longitude = 0.0;
    String ssid; // changed from routerName to ssid
    String ipAddress;
    String cityName;
    std::vector<String> oepnvFilters; // e.g. {"RE", "S-Bahn", "Bus"}
    std::vector<String> stopIds; // all found stop IDs
    std::vector<String> stopNames; // all found stop names
    std::vector<int> stopDistances; // distances to stops
    String selectedStopId = ""; // User's selected stop ID from config
    String selectedStopName = ""; // User's selected stop name from config

    // Configuration phase tracking
    bool wifiConfigured = false; // true when WiFi credentials are validated AND internet is accessible

    // New configuration values from the updated web interface
    int weatherInterval = 3; // Weather update interval in hours (default: 3)
    int transportInterval = 3; // Transport update interval in minutes (default: 3)
    String transportActiveStart = "06:00"; // Active time start for transport updates
    String transportActiveEnd = "09:00"; // Active time end for transport updates
    int walkingTime = 5; // Walking time to stop in minutes (default: 5)
    String sleepStart = "22:30"; // Deep sleep start time
    String sleepEnd = "05:30"; // Deep sleep end time
    bool weekendMode = false; // Enable different weekend settings
    String weekendTransportStart = "08:00"; // Weekend transport active start
    String weekendTransportEnd = "20:00"; // Weekend transport active end
    String weekendSleepStart = "23:00"; // Weekend sleep start
    String weekendSleepEnd = "07:00"; // Weekend sleep end

    // OTA firmware update configuration
    bool otaEnabled = true; // Enable automatic OTA firmware updates (default: enabled)
    String otaCheckTime = "03:00"; // Time to check for OTA updates daily (default: 3:00 AM)
};

