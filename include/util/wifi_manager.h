#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

class MyWiFiManager {
public:
  static void reconnectWiFi();
  // NEW: Refactored WiFi setup functions for clearer control flow
  static void setupWiFiAccessPointAndRestart(WiFiManager& wm);

  static bool isConnected();
  static String getLocalIP();

  // WiFi and internet validation for configuration phase tracking
  static bool hasInternetAccess();

private:
  static const int FULL_CONNECT_TIMEOUT_MS = 10000; // 10 seconds
};
