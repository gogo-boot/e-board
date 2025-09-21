#include "display/text_utils.h"
#include <esp_log.h>

static const char* TAG = "TEXT_UTILS";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* TextUtils::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX* TextUtils::u8g2 = nullptr;

void TextUtils::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& displayRef,
                     U8G2_FOR_ADAFRUIT_GFX& u8g2Ref) {
    display = &displayRef;
    u8g2 = &u8g2Ref;

    ESP_LOGI(TAG, "TextUtils initialized");
}

// Font functions with descriptive names including pixel size and margin
void TextUtils::setFont8px_margin10px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB08_tf); // 8pt Helvetica Bold - ~8px height, needs 10px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setFont10px_margin12px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB10_tf); // 10pt Helvetica Bold - ~10px height, needs 12px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setFont12px_margin15px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB12_tf); // 12pt Helvetica Bold - ~12px height, needs 15px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setFont14px_margin17px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB14_tf); // 14pt Helvetica Bold - ~14px height, needs 17px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setFont18px_margin22px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB18_tf); // 18pt Helvetica Bold - ~18px height, needs 22px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setFont24px_margin28px() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }

    u8g2->setFont(u8g2_font_helvB24_tf); // 24pt Helvetica Bold - ~24px height, needs 28px margin
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

// Font metrics utilities
int16_t TextUtils::getCurrentFontHeight() {
    if (!u8g2) return 0;
    return u8g2->getFontAscent() + u8g2->getFontDescent();
}

int16_t TextUtils::getCurrentFontAscent() {
    if (!u8g2) return 0;
    return u8g2->getFontAscent();
}

int16_t TextUtils::getCurrentFontDescent() {
    if (!u8g2) return 0;
    return u8g2->getFontDescent();
}

// Helper function for proper text positioning
void TextUtils::printTextAtWithMargin(int16_t x, int16_t y, const String& text) {
    if (!u8g2) return;

    // The y coordinate should already include proper margin, so use as-is
    u8g2->setCursor(x, y + getCurrentFontAscent());
    u8g2->print(text);
}

// Helper function to position text with top margin (more intuitive)
void TextUtils::printTextAtTopMargin(int16_t x, int16_t topY, const String& text) {
    if (!u8g2) return;

    // Calculate baseline from desired top position
    int16_t baseline = topY + getCurrentFontAscent();
    u8g2->setCursor(x, baseline);
    u8g2->print(text);
}

// Helper function to print strikethrough text
void TextUtils::printStrikethroughTextAtTopMargin(int16_t x, int16_t topY, const String& text) {
    if (!u8g2 || !display) return;

    // First print the text normally
    printTextAtTopMargin(x, topY, text);

    // Calculate strikethrough line position (middle of text height)
    int16_t textWidth = getTextWidth(text);
    int16_t fontHeight = getCurrentFontHeight();
    int16_t strikeY = topY + (getCurrentFontAscent() / 2); // Position line in upper-middle of text

    // Draw strikethrough line
    display->drawLine(x, strikeY, x + textWidth, strikeY, GxEPD_BLACK);
}

// Get current font ascent for positioning calculations
int16_t TextUtils::getFontAscent() {
    if (!u8g2) return 0;
    return u8g2->getFontAscent();
}

int16_t TextUtils::getTextWidth(const String& text) {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return 0;
    }

    return u8g2->getUTF8Width(text.c_str());
}

String TextUtils::shortenTextToFit(const String& text, int16_t maxWidth) {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return text;
    }

    if (getTextWidth(text) <= maxWidth) {
        return text; // Text fits as-is
    }

    // Try progressively shorter versions with "..."
    String ellipsis = "...";
    int16_t ellipsisWidth = getTextWidth(ellipsis);

    if (maxWidth <= ellipsisWidth) {
        return ""; // Not enough space even for ellipsis
    }

    int16_t availableWidth = maxWidth - ellipsisWidth;

    // Binary search for the longest fitting substring
    int left = 0;
    int right = text.length();
    int bestLength = 0;

    while (left <= right) {
        int mid = (left + right) / 2;
        String testText = text.substring(0, mid);

        if (getTextWidth(testText) <= availableWidth) {
            bestLength = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    if (bestLength == 0) {
        return ellipsis;
    }

    return text.substring(0, bestLength) + ellipsis;
}
