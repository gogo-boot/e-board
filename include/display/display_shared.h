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
    static void init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& displayRef,
                     U8G2_FOR_ADAFRUIT_GFX& u8g2Ref,
                     int16_t screenW, int16_t screenH);

    /**
     * @brief Get display reference
     */
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* getDisplay();

    /**
     * @brief Get U8G2 reference
     */
    static U8G2_FOR_ADAFRUIT_GFX* getU8G2();

    /**
     * @brief Get screen dimensions
     */
    static int16_t getScreenWidth();
    static int16_t getScreenHeight();

private:
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* display;
    static U8G2_FOR_ADAFRUIT_GFX* u8g2;
    static int16_t screenWidth;
    static int16_t screenHeight;
};

#endif // DISPLAY_SHARED_H
