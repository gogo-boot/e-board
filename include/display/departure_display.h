#ifndef DEPARTURE_DISPLAY_H
#define DEPARTURE_DISPLAY_H

#include <Arduino.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/config_manager.h"
#include "api/rmv_api.h"

/**
 * @brief Departure Display Functions
 * 
 * Handles all departure-related drawing functionality for the e-paper display.
 * Separated from DisplayManager to maintain clean code organization.
 */
class DepartureDisplay {
public:
    /**
     * @brief Initialize DepartureDisplay with display references
     */
    static void init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                     U8G2_FOR_ADAFRUIT_GFX &u8g2Ref, 
                     int16_t screenW, int16_t screenH);

    /**
     * @brief Draw the complete departure section
     */
    static void drawDepartureSection(const DepartureData &departures, int16_t x, int16_t y, int16_t w, int16_t h);

    /**
     * @brief Draw the departure footer with timestamp
     */
    static void drawDepartureFooter(int16_t x, int16_t y, int16_t h);

    /**
     * @brief Draw a single departure entry
     */
    static void drawSingleDeparture(const DepartureInfo &dep, int16_t leftMargin, int16_t rightMargin, 
                                  int16_t currentY, bool isFullScreen);

    /**
     * @brief Extract stop name from config format
     */
    static String getStopName(RTCConfigData &config);

private:
    // Static display references
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *display;
    static U8G2_FOR_ADAFRUIT_GFX *u8g2;
    static int16_t screenWidth;
    static int16_t screenHeight;

    /**
     * @brief Draw departures in full screen mode
     */
    static void drawFullScreenDepartures(const DepartureData &departures, int16_t leftMargin, 
                                       int16_t rightMargin, int16_t &currentY, int16_t y, int16_t h);

    /**
     * @brief Draw departures in half screen mode (separated by direction)
     */
    static void drawHalfScreenDepartures(const DepartureData &departures, int16_t leftMargin, 
                                       int16_t rightMargin, int16_t currentY, int16_t y, int16_t h);
};

#endif // DEPARTURE_DISPLAY_H
