#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

class MyWiFiManager {
public:
    static void reconnectWiFi();
    static void setupAPMode(WiFiManager &wm);
    static bool isConnected();
    static String getLocalIP();
};
