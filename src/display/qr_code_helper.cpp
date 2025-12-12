#include "display/qr_code_helper.h"

#include <Arduino.h>
#include <esp_log.h>
#include <qrcode.h>
#include "global_instances.h"

static const char* TAG = "QR_HELPER";

bool QRCodeHelper::drawQRCode(int16_t x, int16_t y, const String& data, uint8_t scale, uint8_t version) {
    ESP_LOGI(TAG, "Drawing QR code at (%d, %d), version %d, scale %d, data: %s",
             x, y, version, scale, data.c_str());

    // Create QR code object
    QRCode qrcode;

    // Calculate buffer size needed for the QR code
    uint8_t qrcodeData[qrcode_getBufferSize(version)];

    // Generate QR code with ECC_MEDIUM (15% error correction)
    int8_t result = qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, data.c_str());

    if (result != 0) {
        ESP_LOGE(TAG, "Failed to generate QR code, error: %d", result);
        return false;
    }

    ESP_LOGI(TAG, "QR code generated - Size: %d modules, Data length: %d bytes",
             qrcode.size, data.length());

    // Draw white background (border around QR code)
    int16_t border = scale * 2; // 2-module border
    int16_t totalSize = (qrcode.size * scale) + (border * 2);
    display.fillRect(x, y, totalSize, totalSize, GxEPD_WHITE);

    // Draw QR code modules
    for (uint8_t row = 0; row < qrcode.size; row++) {
        for (uint8_t col = 0; col < qrcode.size; col++) {
            if (qrcode_getModule(&qrcode, col, row)) {
                // Draw black module (filled rectangle)
                int16_t moduleX = x + border + (col * scale);
                int16_t moduleY = y + border + (row * scale);
                display.fillRect(moduleX, moduleY, scale, scale, GxEPD_BLACK);
            }
        }
    }

    // Draw border around QR code
    display.drawRect(x, y, totalSize, totalSize, GxEPD_BLACK);

    ESP_LOGI(TAG, "QR code drawn successfully - Total size: %dx%d pixels", totalSize, totalSize);
    return true;
}

int16_t QRCodeHelper::getQRCodeSize(uint8_t version, uint8_t scale) {
    // QR code size formula: (version * 4) + 17
    uint8_t moduleCount = (version * 4) + 17;
    int16_t border = scale * 2; // 2-module border
    return (moduleCount * scale) + (border * 2);
}

void QRCodeHelper::drawQRLabel(int16_t x, int16_t y, int16_t qrSize, const String& text, int16_t offsetY) {
    // Calculate center position for text
    int16_t centerX = x + (qrSize / 2);
    int16_t textY = y + qrSize + offsetY;

    // Set font for label
    u8g2.setFont(u8g2_font_helvB10_tf); // Bold 10pt
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);

    // Get text bounds for centering
    int16_t textWidth = u8g2.getUTF8Width(text.c_str());
    int16_t textX = centerX - (textWidth / 2);

    // Draw text
    u8g2.setCursor(textX, textY);
    u8g2.print(text);
    ESP_LOGI(TAG, "QR label drawn: '%s' at (%d, %d)", text.c_str(), textX, textY);
}


