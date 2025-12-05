#pragma once

#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

/**
 * Boot Flow Manager
 *
 * Orchestrates the 3-phase boot process:
 * - PHASE 1: WiFi Setup (first boot, no WiFi configured)
 * - PHASE 2: App Setup (WiFi configured, app settings needed)
 * - PHASE 3: Complete (fully configured, operational mode)
 *
 * Also handles:
 * - Display mode selection (half-and-half, weather-only, departure-only)
 * - WiFi validation and fallback
 * - Operational mode execution
 */
namespace BootFlowManager {
    /**
     * Execute the appropriate boot flow based on current configuration phase
     * - Detects current phase
     * - Handles phase transitions
     * - Executes operational mode if fully configured
     */
    void handlePhaseWifiSetup();
    void handlePhaseAppSetup();
    void handlePhaseComplete();
}

