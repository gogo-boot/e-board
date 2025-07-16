#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <Arduino.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

/**
 * @brief Text and Font Utility Functions
 * 
 * This class provides shared text rendering and font management utilities
 * that can be used across different display components (weather, departures, etc.)
 */
class TextUtils {
public:
    /**
     * @brief Initialize TextUtils with display references
     * @param displayRef Reference to the e-paper display
     * @param u8g2Ref Reference to the U8G2 font renderer
     */
    static void init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                     U8G2_FOR_ADAFRUIT_GFX &u8g2Ref);

    // Font management functions
    static void setLargeFont();
    static void setMediumFont();
    static void setSmallFont();

    // Text measurement and fitting functions
    static int16_t getTextWidth(const String& text);
    static String shortenTextToFit(const String& text, int16_t maxWidth);

private:
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *display;
    static U8G2_FOR_ADAFRUIT_GFX *u8g2;
};

#endif // TEXT_UTILS_H
