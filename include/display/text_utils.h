#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <Arduino.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

/**
 * @brief Text and Font Utility Functions
 * 
 * Font function naming convention: setFont[SIZE]px_margin[MARGIN]px()
 * - SIZE: actual font height in pixels
 * - MARGIN: recommended top margin to prevent text clipping
 * 
 * Example: setFont12px_margin15px() means 12px font with 15px top margin
 */
class TextUtils {
public:
    /**
     * @brief Initialize TextUtils with display references
     */
    static void init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                     U8G2_FOR_ADAFRUIT_GFX &u8g2Ref);

    // Font functions with pixel sizes and recommended margins
    static void setFont8px_margin10px();   // Very small font - 8px height, 10px margin
    static void setFont10px_margin12px();  // Small font - 10px height, 12px margin
    static void setFont12px_margin15px();  // Medium font - 12px height, 15px margin
    static void setFont14px_margin17px();  // Medium-large font - 14px height, 17px margin
    static void setFont18px_margin22px();  // Extra large font - 18px height, 22px margin
    static void setFont24px_margin28px();  // Huge font - 24px height, 28px margin

    // Text measurement and fitting functions
    static int16_t getTextWidth(const String& text);
    static String shortenTextToFit(const String& text, int16_t maxWidth);

    // Font metrics utilities
    static int16_t getCurrentFontHeight();
    static int16_t getCurrentFontAscent();
    static int16_t getCurrentFontDescent();

    // Helper function for proper text positioning
    static void printTextAtWithMargin(int16_t x, int16_t y, const String& text);

private:
    static GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *display;
    static U8G2_FOR_ADAFRUIT_GFX *u8g2;
};

#endif // TEXT_UTILS_H
