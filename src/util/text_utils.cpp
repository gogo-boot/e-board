#include "util/text_utils.h"
#include <esp_log.h>

static const char *TAG = "TEXT_UTILS";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> *TextUtils::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX *TextUtils::u8g2 = nullptr;

void TextUtils::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> &displayRef, 
                     U8G2_FOR_ADAFRUIT_GFX &u8g2Ref) {
    display = &displayRef;
    u8g2 = &u8g2Ref;
    
    ESP_LOGI(TAG, "TextUtils initialized");
}

void TextUtils::setLargeFont() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }
    
    u8g2->setFont(u8g2_font_helvB18_tf); // 18pt Helvetica Bold with German support
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setMediumFont() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }
    
    u8g2->setFont(u8g2_font_helvB12_tf); // 12pt Helvetica Bold with German support
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
}

void TextUtils::setSmallFont() {
    if (!u8g2) {
        ESP_LOGE(TAG, "TextUtils not initialized! Call init() first.");
        return;
    }
    
    u8g2->setFont(u8g2_font_helvB10_tf); // 10pt Helvetica Bold with German support
    u8g2->setForegroundColor(GxEPD_BLACK);
    u8g2->setBackgroundColor(GxEPD_WHITE);
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
