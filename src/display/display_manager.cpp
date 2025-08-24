#include "display/display_manager.h"

#include <Arduino.h>
#include <esp_log.h>

#include "config/config_manager.h"
#include "display/text_utils.h"
#include "display/departure_display.h"
#include "display/weather_general_half.h"
#include "display/weather_general_full.h"
#include "display/weather_graph.h"
#include "display/display_shared.h"
#include "util/time_manager.h"

// Include e-paper display libraries
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

#include "config/pins.h"

// Font includes for German character support
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char* TAG = "DISPLAY_MGR";

// Static member variables
DisplayMode DisplayManager::currentMode = DisplayMode::HALF_AND_HALF;
DisplayOrientation DisplayManager::currentOrientation =
  DisplayOrientation::LANDSCAPE;
int16_t DisplayManager::screenWidth = 0; // Will be read from display
int16_t DisplayManager::screenHeight = 0; // Will be read from display
int16_t DisplayManager::halfWidth = 0; // Will be calculated
int16_t DisplayManager::halfHeight = 0; // Will be calculated

void DisplayManager::init(DisplayOrientation orientation) {
  ESP_LOGI(TAG, "Initializing display manager");

  display.init(115200);

  // Initialize U8g2 for UTF-8 font support (German umlauts)
  u8g2.begin(display);
  u8g2.setFontMode(1); // Use u8g2 transparent mode
  u8g2.setFontDirection(0); // Left to right
  u8g2.setForegroundColor(GxEPD_BLACK);
  u8g2.setBackgroundColor(GxEPD_WHITE);

  // Set rotation first to get correct dimensions
  display.setRotation(static_cast<int>(orientation));

  // Now get dimensions from display after rotation is set
  screenWidth = display.width();
  screenHeight = display.height();

  // Initialize shared display resources once for all display components
  DisplayShared::init(display, u8g2, screenWidth, screenHeight);

  // Initialize TextUtils with shared resources
  TextUtils::init(display, u8g2);

  ESP_LOGI(TAG, "Display detected - Physical dimensions: %dx%d", screenWidth,
           screenHeight);

  setMode(DisplayMode::HALF_AND_HALF, orientation);

  ESP_LOGI(
    TAG,
    "Display initialized - Orientation: Landscape, Active dimensions: %dx%d",
    screenWidth, screenHeight);
}

void DisplayManager::setMode(DisplayMode mode, DisplayOrientation orientation) {
  currentMode = mode;
  currentOrientation = orientation;

  // Set display rotation
  display.setRotation(static_cast<int>(orientation));

  // Get dimensions after rotation
  screenWidth = display.width();
  screenHeight = display.height();

  calculateDimensions();

  ESP_LOGI(
    TAG,
    "Display mode set - Mode: %d, Orientation: Landscape, Dimensions: %dx%d",
    static_cast<int>(mode), screenWidth, screenHeight);
}

void DisplayManager::calculateDimensions() {
  // Landscape mode: 800x480 - split WIDTH in half
  // Weather: left half (0-399), Departures: right half (400-799)
  halfWidth = screenWidth / 2; // Split width: 400 pixels each
  halfHeight = screenHeight; // Full height: 480 pixels
  ESP_LOGI(TAG, "Landscape split: Weather[0,0,%d,%d] Departures[%d,0,%d,%d]",
           halfWidth, screenHeight, halfWidth, halfWidth, screenHeight);
}

void DisplayManager::displayHalfAndHalf(const WeatherInfo* weather,
                                        const DepartureData* departures) {
  ESP_LOGI(TAG, "Displaying half and half mode");

  if (!weather && !departures) {
    ESP_LOGW(TAG, "No data provided for half and half display");
    return;
  }

  // Remove header - use full screen height
  const int16_t footerHeight = 15;
  const int16_t contentY = 0; // Start from top instead of after header
  const int16_t contentHeight = screenHeight - footerHeight;

  if (weather->hourlyForecastCount > 0 && departures->departureCount > 0) {
    // Full update - both halves without header
    ESP_LOGI(TAG, "Updating both halves without header");

    bool isFullUpdate = true; // Full update for both halves

    // Set the display update region to the entire screen.
    // This ensures all drawing operations affect the whole display.
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      // Remove header drawing
      // Landscape: left/right split (weather left, departures right)
      updateWeatherHalf(isFullUpdate, *weather);
      updateDepartureHalf(isFullUpdate, *departures);

      // Draw vertical divider
      displayVerticalLine(contentY);
    } while (display.nextPage());
  } else if (weather->hourlyForecastCount > 0) {
    display.firstPage();
    // Partial update - weather half only
    do {
      updateWeatherHalf(false, *weather);
    } while (display.nextPage());
  } else if (departures->departureCount > 0) {
    display.firstPage();
    do {
      // Partial update - departure half only
      // updateDepartureHalf(false, *departures);
      display.setPartialWindow(halfWidth + 1, contentHeight, halfWidth - 1,
                               contentHeight);
      updateDepartureHalf(false, *departures);
    } while (display.nextPage());
  }
}

void DisplayManager::displayVerticalLine(const int16_t contentY) {
  display.drawLine(halfWidth, contentY, halfWidth, screenHeight, GxEPD_BLACK);
}

void DisplayManager::updateWeatherHalf(bool isFullUpate,
                                       const WeatherInfo& weather) {
  ESP_LOGI(TAG, "Updating weather half");

  const int16_t footerHeight = 15;
  const int16_t contentY = 0; // Start from top without header
  const int16_t contentHeight = screenHeight; // Use full height

  // Landscape: weather is LEFT half (full height)
  int16_t x = 0, y = 0, w = halfWidth, h = screenHeight;

  if (!isFullUpate) {
    // Use setPartialWindow(x, y, w, h) for partial updates instead.
    display.setPartialWindow(x, y, w, h);
  }

  int16_t leftMargin = x + 10;
  int16_t rightMargin = x + w - 10;
  // Use partial window for faster update
  WeatherHalfDisplay::drawHalfScreenWeatherLayout(weather, leftMargin, rightMargin, y, contentHeight);
  WeatherHalfDisplay::drawWeatherFooter(x, screenHeight - footerHeight, 15);
}

void DisplayManager::updateDepartureHalf(bool isFullUpate,
                                         const DepartureData& departures) {
  ESP_LOGI(TAG, "Updating departure half");

  const int16_t footerHeight = 15;
  const int16_t contentY = 0; // Start from top without header
  const int16_t contentHeight = screenHeight; // Use full height

  if (!isFullUpate) {
    // // Use partial window for faster update
    display.setPartialWindow(halfWidth, contentY, halfWidth, contentHeight);
  }

  DepartureDisplay::drawHalfScreenDepartureSection(
    departures, halfWidth, contentY, halfWidth, contentHeight - footerHeight);
  DepartureDisplay::drawDepartureFooter(halfWidth, screenHeight - footerHeight,
                                        footerHeight);
}

void DisplayManager::displayWeatherFull(const WeatherInfo& weather) {
  ESP_LOGI(TAG, "Displaying weather only mode");

  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    WeatherFullDisplay::drawFullScreenWeatherLayout(weather, 0, 0, screenWidth,
                                                    screenHeight);
  } while (display.nextPage());
}

void DisplayManager::displayDeparturesFull(const DepartureData& departures) {
  ESP_LOGI(TAG, "Displaying departures only mode");

  display.setFullWindow();
  display.firstPage();

  do {
    display.fillScreen(GxEPD_WHITE);
    DepartureDisplay::drawFullScreenDepartureSection(departures, 0, 0,
                                                     screenWidth, screenHeight);
  } while (display.nextPage());
}

// Weather functions moved to WeatherDisplay class
// Departure functions moved to DepartureDisplay class

void DisplayManager::hibernate() {
  ESP_LOGI(TAG, "Hibernating display");

  // Turn off display
  display.hibernate();

  // You can add additional power-saving measures here
  ESP_LOGI(TAG, "Display hibernated");
}
