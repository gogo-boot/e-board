#pragma once

#include <cstdint>
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
     * Initialize boot flow manager with required components
     * @param webServer Reference to web server for configuration mode
     * @param display Reference to e-paper display
     * @param u8g2 Reference to U8g2 font renderer
     */
    void initialize(WebServer& webServer,
                    GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& display,
                    U8G2_FOR_ADAFRUIT_GFX& u8g2);

    /**
     * Execute the appropriate boot flow based on current configuration phase
     * - Detects current phase
     * - Handles phase transitions
     * - Executes operational mode if fully configured
     */
    void handleBootFlow();
    void handlePhaseWifiSetup();
    void handlePhaseAppSetup();
    void handlePhaseComplete();
}

