#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>


class MyWiFiManager {
public:
  static void reconnectWiFi();
  // NEW: Refactored WiFi setup functions for clearer control flow
  static void setupWiFiAccessPointAndRestart(WiFiManager& wm);

  static bool isConnected();
  static String getLocalIP();

  // Fast WiFi reconnection methods for deep sleep optimization
  static bool fastReconnectWiFi();
  static void saveWiFiStateToRTC();
  static bool isWiFiStateCached();
  static void clearWiFiCache();

  // WiFi and internet validation for configuration phase tracking
  static bool hasInternetAccess();

private:
  static const int FAST_CONNECT_TIMEOUT_MS = 8000; // 8 seconds for fast connect
  static const int FULL_CONNECT_TIMEOUT_MS = 20000; // 20 seconds for full scan
};
