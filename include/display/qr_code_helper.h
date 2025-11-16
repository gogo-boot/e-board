#pragma once

#include <Arduino.h>

/**
 * QR Code Helper - Utility class for generating and drawing QR codes on e-paper display
 * Uses ricmoo/QRCode library with ECC_MEDIUM error correction
 */
class QRCodeHelper {
public:
    /**
     * Draw a QR code on the display at specified position
     * @param x Top-left X coordinate
     * @param y Top-left Y coordinate
     * @param data String data to encode in QR code
     * @param scale Pixel size multiplier (e.g., 3 = 3x3 pixels per module)
     * @param version QR code version (1-40), determines module count
     * @return true if successful, false on error
     */
    static bool drawQRCode(int16_t x, int16_t y, const String& data, uint8_t scale = 4, uint8_t version = 3);

    /**
     * Calculate the pixel size of a QR code
     * @param version QR code version
     * @param scale Pixel size multiplier
     * @return Total pixel size (width/height)
     */
    static int16_t getQRCodeSize(uint8_t version, uint8_t scale);

    /**
     * Draw text centered below a QR code
     * @param x Top-left X coordinate of QR code
     * @param y Top-left Y coordinate of QR code
     * @param qrSize QR code pixel size
     * @param text Text to display
     * @param offsetY Vertical offset below QR code
     */
    static void drawQRLabel(int16_t x, int16_t y, int16_t qrSize, const String& text, int16_t offsetY = 10);
};

