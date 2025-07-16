#include "display/display_manager.h"
#include "display/weather_display.h"
#include "display/departure_display.h"
#include "display/text_utils.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"
#include "util/time_manager.h"
#include "display/weather_graph.h"

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/pins.h"

// Font includes for German character support
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char *TAG = "DISPLAY_MGR";

// Static member variables
DisplayMode DisplayManager::currentMode = DisplayMode::HALF_AND_HALF;
DisplayOrientation DisplayManager::currentOrientation = DisplayOrientation::LANDSCAPE;
int16_t DisplayManager::screenWidth = 0;  // Will be read from display
int16_t DisplayManager::screenHeight = 0; // Will be read from display
int16_t DisplayManager::halfWidth = 0;    // Will be calculated
int16_t DisplayManager::halfHeight = 0;   // Will be calculated

void DisplayManager::init(DisplayOrientation orientation)
{
    ESP_LOGI(TAG, "Initializing display manager");

    display.init(115200);

    // Initialize U8g2 for UTF-8 font support (German umlauts)
    u8g2.begin(display);
    u8g2.setFontMode(1);      // Use u8g2 transparent mode
    u8g2.setFontDirection(0); // Left to right
    u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setBackgroundColor(GxEPD_WHITE);

    // Set rotation first to get correct dimensions
    display.setRotation(static_cast<int>(orientation));

    // Now get dimensions from display after rotation is set
    screenWidth = display.width();
    screenHeight = display.height();

    // Initialize WeatherDisplay with shared resources
    WeatherDisplay::init(display, u8g2, screenWidth, screenHeight);

    // Initialize DepartureDisplay with shared resources
    DepartureDisplay::init(display, u8g2, screenWidth, screenHeight);

    // Initialize TextUtils with shared resources
    TextUtils::init(display, u8g2);

    ESP_LOGI(TAG, "Display detected - Physical dimensions: %dx%d", screenWidth, screenHeight);

    setMode(DisplayMode::HALF_AND_HALF, orientation);

    ESP_LOGI(TAG, "Display initialized - Orientation: Landscape, Active dimensions: %dx%d",
             screenWidth, screenHeight);
}

void DisplayManager::setMode(DisplayMode mode, DisplayOrientation orientation)
{
    currentMode = mode;
    currentOrientation = orientation;

    // Set display rotation
    display.setRotation(static_cast<int>(orientation));

    // Get dimensions after rotation
    screenWidth = display.width();
    screenHeight = display.height();

    calculateDimensions();

    ESP_LOGI(TAG, "Display mode set - Mode: %d, Orientation: Landscape, Dimensions: %dx%d",
             static_cast<int>(mode),
             screenWidth, screenHeight);
}

void DisplayManager::calculateDimensions()
{
    // Landscape mode: 800x480 - split WIDTH in half
    // Weather: left half (0-399), Departures: right half (400-799)
    halfWidth = screenWidth / 2; // Split width: 400 pixels each
    halfHeight = screenHeight;   // Full height: 480 pixels
    ESP_LOGI(TAG, "Landscape split: Weather[0,0,%d,%d] Departures[%d,0,%d,%d]",
             halfWidth, screenHeight, halfWidth, halfWidth, screenHeight);
}

void DisplayManager::displayHalfAndHalf(const WeatherInfo *weather, const DepartureData *departures)
{
    ESP_LOGI(TAG, "Displaying half and half mode");

    if (!weather && !departures)
    {
        ESP_LOGW(TAG, "No data provided for half and half display");
        return;
    }

    // Remove header - use full screen height
    const int16_t footerHeight = 15;
    const int16_t contentY = 0; // Start from top instead of after header
    const int16_t contentHeight = screenHeight - footerHeight;

    if (weather && departures)
    {
        // Full update - both halves without header
        ESP_LOGI(TAG, "Updating both halves without header");

        bool isFullUpdate = true; // Full update for both halves
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);

            // Remove header drawing
            // Landscape: left/right split (weather left, departures right)
            updateWeatherHalf(isFullUpdate, *weather);
            updateDepartureHalf(isFullUpdate, *departures);

            // Todo Draw it only in debug mode
            displayVerticalLine(isFullUpdate, contentY);
        } while (display.nextPage());
    }
    else if (weather)
    {
        display.setFullWindow();
        display.firstPage();
        // Partial update - weather half only
        do
        {
            updateWeatherHalf(false, *weather);
        } while (display.nextPage());
    }
    else if (departures)
    {
        display.setFullWindow();
        display.firstPage();
        do
        {
            // Partial update - departure half only
            updateDepartureHalf(false, *departures);
        } while (display.nextPage());
    }
}

void DisplayManager::displayVerticalLine(bool isFoolUpdate, const int16_t contentY)
{
    if (!isFoolUpdate)
    {
        display.setPartialWindow(halfWidth, contentY, 1, screenHeight - contentY);
        display.fillRect(halfWidth, contentY, 1, screenHeight - contentY, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping vertical line update in non-full update mode");
        return;
    }
    // Draw vertical divider
    display.drawLine(halfWidth, contentY, halfWidth, screenHeight, GxEPD_BLACK);
}

void DisplayManager::updateWeatherHalf(bool isFullUpate, const WeatherInfo &weather)
{
    ESP_LOGI(TAG, "Updating weather half");

    const int16_t footerHeight = 25;
    const int16_t contentY = 0; // Start from top without header
    const int16_t contentHeight = screenHeight; // Use full height

    // Landscape: weather is LEFT half (full height)
    int16_t x = 0, y = 0, w = halfWidth, h = screenHeight;

    if (!isFullUpate)
    {
        display.setPartialWindow(x, y, w, h);

        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping weather half update in non-full update mode");
        return;
    }
    // Use partial window for faster update
    WeatherDisplay::drawWeatherSection(weather, x, contentY, w, contentHeight);
    WeatherDisplay::drawWeatherFooter(x, screenHeight - footerHeight);
}

void DisplayManager::updateDepartureHalf(bool isFullUpate,const DepartureData &departures)
{
    ESP_LOGI(TAG, "Updating departure half");

    const int16_t footerHeight = 15;
    const int16_t contentY = 0; // Start from top without header
    const int16_t contentHeight = screenHeight; // Use full height

    // Landscape: departures are RIGHT half (full height)
    int16_t x = halfWidth, y = contentY, w = halfWidth, h = contentHeight;

    if (!isFullUpate)
    {
        // // Use partial window for faster update
        display.setPartialWindow(x, y, w, h);

        display.fillRect(x, y, w, h, GxEPD_WHITE);
        ESP_LOGI(TAG, "Skipping departure half update in non-full update mode");
        return;
    }
    DepartureDisplay::drawDepartureSection(departures, x, y, w, h);
    DepartureDisplay::drawDepartureFooter(x, screenHeight - footerHeight);
    // Redraw vertical divider
    display.drawLine(halfWidth, 0, halfWidth, screenHeight, GxEPD_BLACK);
}

void DisplayManager::displayWeatherOnly(const WeatherInfo &weather)
{
    ESP_LOGI(TAG, "Displaying weather only mode");

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        WeatherDisplay::drawWeatherSection(weather, 0, 0, screenWidth, screenHeight);
    } while (display.nextPage());
}

void DisplayManager::displayDeparturesOnly(const DepartureData &departures)
{
    ESP_LOGI(TAG, "Displaying departures only mode");

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        DepartureDisplay::drawDepartureSection(departures, 0, 0, screenWidth, screenHeight);
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
