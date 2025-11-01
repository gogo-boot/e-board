#ifndef TRANSPORT_DISPLAY_H
#define TRANSPORT_DISPLAY_H

#include <Arduino.h>
#include "config/config_manager.h"
#include "api/rmv_api.h"

/**
 * @brief Transport Display Functions
 *
 * Handles all transport-related drawing functionality for the e-paper display.
 * Separated from DisplayManager to maintain clean code organization.
 */
class TransportDisplay {
public:
    /**
     * @brief Draw the complete transport section
     */
    static void drawHalfScreenTransportSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                               int16_t h);

    static void drawFullScreenTransportSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w,
                                               int16_t h);

    /**
     * @brief Draw the transport footer with timestamp
     */
    static void drawTransportFooter(int16_t x, int16_t y, int16_t h);

private:
    /**
     * @brief Draw transports in half screen mode (separated by direction)
     */
    static void drawHalfScreenTransports(const DepartureData& departures, int16_t leftMargin,
                                         int16_t rightMargin, int16_t currentY, int16_t h);
    static void drawTransportList(std::vector<const DepartureInfo*> departure, int16_t x,
                                  int16_t y, int16_t w, int16_t h, bool printLabel, int maxPerDirection);
    static void getSeparatedTransportDirection(const DepartureData& departures,
                                               std::vector<const DepartureInfo*>& direction1Departures,
                                               std::vector<const DepartureInfo*>& direction2Departures);

    /**
     * @brief Draw a single transport entry
     */
    static void drawSingleTransport(const DepartureInfo& dep, int16_t x, int16_t width,
                                    int16_t currentY);
};

#endif // TRANSPORT_DISPLAY_H
