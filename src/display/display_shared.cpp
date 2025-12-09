#include "display/display_shared.h"
#include <esp_log.h>

static const char* TAG = "DISPLAY_SHARED";

// Static member initialization
int16_t DisplayShared::screenWidth = 0;
int16_t DisplayShared::screenHeight = 0;

void DisplayShared::init(int16_t screenW, int16_t screenH) {
    screenWidth = screenW;
    screenHeight = screenH;

    ESP_LOGI(TAG, "Shared display resources initialized with screen size %dx%d", screenW, screenH);
}

int16_t DisplayShared::getScreenWidth() {
    return screenWidth;
}

int16_t DisplayShared::getScreenHeight() {
    return screenHeight;
}
