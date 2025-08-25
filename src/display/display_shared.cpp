#include "display/display_shared.h"
#include <esp_log.h>

static const char* TAG = "DISPLAY_SHARED";

// Static member initialization
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* DisplayShared::display = nullptr;
U8G2_FOR_ADAFRUIT_GFX* DisplayShared::u8g2 = nullptr;
int16_t DisplayShared::screenWidth = 0;
int16_t DisplayShared::screenHeight = 0;

void DisplayShared::init(GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>& displayRef,
                         U8G2_FOR_ADAFRUIT_GFX& u8g2Ref,
                         int16_t screenW, int16_t screenH) {
    display = &displayRef;
    u8g2 = &u8g2Ref;
    screenWidth = screenW;
    screenHeight = screenH;

    ESP_LOGI(TAG, "Shared display resources initialized with screen size %dx%d", screenW, screenH);
}

GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT>* DisplayShared::getDisplay() {
    return display;
}

U8G2_FOR_ADAFRUIT_GFX* DisplayShared::getU8G2() {
    return u8g2;
}

int16_t DisplayShared::getScreenWidth() {
    return screenWidth;
}

int16_t DisplayShared::getScreenHeight() {
    return screenHeight;
}
