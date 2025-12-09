#ifndef DISPLAY_SHARED_H
#define DISPLAY_SHARED_H

#include <Arduino.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

/**
 * @brief Shared display resources for all display components
 *
 * Provides centralized access to display objects and screen dimensions
 * for all weather and departure display classes.
 */
class DisplayShared {
public:
    /**
     * @brief Initialize shared display resources
     */
    static void init(int16_t screenW, int16_t screenH);

    /**
     * @brief Get screen dimensions
     */
    static int16_t getScreenWidth();
    static int16_t getScreenHeight();

private:
    static int16_t screenWidth;
    static int16_t screenHeight;
};

#endif // DISPLAY_SHARED_H
