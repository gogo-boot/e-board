#include "util/display_manager.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/pins.h"

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;

static const char* TAG = "DISPLAY_MGR";

// Static member variables
DisplayMode DisplayManager::currentMode = DisplayMode::HALF_AND_HALF;
DisplayOrientation DisplayManager::currentOrientation = DisplayOrientation::LANDSCAPE;
int16_t DisplayManager::screenWidth = 0;   // Will be read from display
int16_t DisplayManager::screenHeight = 0;  // Will be read from display
int16_t DisplayManager::halfWidth = 0;     // Will be calculated
int16_t DisplayManager::halfHeight = 0;    // Will be calculated

void DisplayManager::init(DisplayOrientation orientation) {
    ESP_LOGI(TAG, "Initializing display manager");
    
    display.init(115200);
    
    // Set rotation first to get correct dimensions
    display.setRotation(static_cast<int>(orientation));
    
    // Now get dimensions from display after rotation is set
    screenWidth = display.width();
    screenHeight = display.height();
    
    ESP_LOGI(TAG, "Display detected - Physical dimensions: %dx%d", screenWidth, screenHeight);
    
    setMode(DisplayMode::HALF_AND_HALF, orientation);
    
    ESP_LOGI(TAG, "Display initialized - Orientation: %s, Active dimensions: %dx%d", 
             orientation == DisplayOrientation::LANDSCAPE ? "Landscape" : "Portrait",
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
    
    ESP_LOGI(TAG, "Display mode set - Mode: %d, Orientation: %s, Dimensions: %dx%d", 
             static_cast<int>(mode),
             orientation == DisplayOrientation::LANDSCAPE ? "Landscape" : "Portrait",
             screenWidth, screenHeight);
}

void DisplayManager::calculateDimensions() {
    // Dimensions are already set from display.width() and display.height() after rotation
    
    if (currentOrientation == DisplayOrientation::LANDSCAPE) {
        // Landscape mode: 800x480 - split WIDTH in half
        // Weather: left half (0-399), Departures: right half (400-799)
        halfWidth = screenWidth / 2;   // Split width: 400 pixels each
        halfHeight = screenHeight;     // Full height: 480 pixels
        ESP_LOGI(TAG, "Landscape split: Weather[0,0,%d,%d] Departures[%d,0,%d,%d]", 
                 halfWidth, screenHeight, halfWidth, halfWidth, screenHeight);
    } else {
        // Portrait mode: 480x800 - split HEIGHT in half  
        // Weather: top half (0-399), Departures: bottom half (400-799)
        halfWidth = screenWidth;       // Full width: 480 pixels
        halfHeight = screenHeight / 2; // Split height: 400 pixels each
        ESP_LOGI(TAG, "Portrait split: Weather[0,0,%d,%d] Departures[0,%d,%d,%d]", 
                 screenWidth, halfHeight, halfHeight, screenWidth, halfHeight);
    }
}

void DisplayManager::displayHalfAndHalf(const WeatherInfo* weather, const DepartureData* departures) {
    ESP_LOGI(TAG, "Displaying half and half mode");
    
    if (!weather && !departures) {
        ESP_LOGW(TAG, "No data provided for half and half display");
        return;
    }
    
    if (weather && departures) {
        // Full update - both halves
        ESP_LOGI(TAG, "Updating both halves");
        display.setFullWindow();
        display.firstPage();
        
        do {
            display.fillScreen(GxEPD_WHITE);
            
            if (currentOrientation == DisplayOrientation::LANDSCAPE) {
                // Landscape: left/right split (weather left, departures right)
                drawWeatherSection(*weather, 0, 0, halfWidth, screenHeight);
                drawDepartureSection(*departures, halfWidth, 0, halfWidth, screenHeight);
                // Draw vertical divider
                display.drawLine(halfWidth, 0, halfWidth, screenHeight, GxEPD_BLACK);
            } else {
                // Portrait: top/bottom split (weather top, departures bottom)
                drawWeatherSection(*weather, 0, 0, screenWidth, halfHeight);
                drawDepartureSection(*departures, 0, halfHeight, screenWidth, halfHeight);
                // Draw horizontal divider
                display.drawLine(0, halfHeight, screenWidth, halfHeight, GxEPD_BLACK);
            }
            
        } while (display.nextPage());
        
    } else if (weather) {
        // Partial update - weather half only
        updateWeatherHalf(*weather);
    } else if (departures) {
        // Partial update - departure half only
        updateDepartureHalf(*departures);
    }
}

void DisplayManager::updateWeatherHalf(const WeatherInfo& weather) {
    ESP_LOGI(TAG, "Updating weather half");
    
    int16_t x, y, w, h;
    if (currentOrientation == DisplayOrientation::LANDSCAPE) {
        // Landscape: weather is LEFT half
        x = 0; y = 0; w = halfWidth; h = screenHeight;
    } else {
        // Portrait: weather is TOP half
        x = 0; y = 0; w = screenWidth; h = halfHeight;
    }
    
    // Use partial window for faster update
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    
    do {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        drawWeatherSection(weather, x, y, w, h);
        
        // Redraw divider
        if (currentOrientation == DisplayOrientation::LANDSCAPE) {
            display.drawLine(halfWidth, 0, halfWidth, screenHeight, GxEPD_BLACK);
        } else {
            display.drawLine(0, halfHeight, screenWidth, halfHeight, GxEPD_BLACK);
        }
        
    } while (display.nextPage());
}

void DisplayManager::updateDepartureHalf(const DepartureData& departures) {
    ESP_LOGI(TAG, "Updating departure half");
    
    int16_t x, y, w, h;
    if (currentOrientation == DisplayOrientation::LANDSCAPE) {
        // Landscape: departures is RIGHT half
        x = halfWidth; y = 0; w = halfWidth; h = screenHeight;
    } else {
        // Portrait: departures is BOTTOM half
        x = 0; y = halfHeight; w = screenWidth; h = halfHeight;
    }
    
    // Use partial window for faster update
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    
    do {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        drawDepartureSection(departures, x, y, w, h);
        
        // Redraw divider
        if (currentOrientation == DisplayOrientation::LANDSCAPE) {
            display.drawLine(halfWidth, 0, halfWidth, screenHeight, GxEPD_BLACK);
        } else {
            display.drawLine(0, halfHeight, screenWidth, halfHeight, GxEPD_BLACK);
        }
        
    } while (display.nextPage());
}

void DisplayManager::displayWeatherOnly(const WeatherInfo& weather) {
    ESP_LOGI(TAG, "Displaying weather only mode");
    
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen(GxEPD_WHITE);
        drawWeatherSection(weather, 0, 0, screenWidth, screenHeight);
    } while (display.nextPage());
}

void DisplayManager::displayDeparturesOnly(const DepartureData& departures) {
    ESP_LOGI(TAG, "Displaying departures only mode");
    
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen(GxEPD_WHITE);
        drawDepartureSection(departures, 0, 0, screenWidth, screenHeight);
    } while (display.nextPage());
}

void DisplayManager::drawWeatherSection(const WeatherInfo& weather, int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t currentY = y + 25;
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;
    
    // Adaptive font sizes based on available width
    bool isFullScreen = (w >= screenWidth * 0.8);
    bool isHalfScreen = (w >= screenWidth * 0.4);
    
    // Weather title
    if (isFullScreen) {
        setLargeFont();
        display.setCursor(leftMargin, currentY);
        display.print("🌤️ Weather Information");
        currentY += 45;
    } else {
        setMediumFont();
        display.setCursor(leftMargin, currentY);
        display.print("🌤️ Weather");
        currentY += 30;
    }
    
    // Current temperature - make it prominent
    setLargeFont();
    display.setCursor(leftMargin, currentY);
    display.print(weather.temperature);
    
    // Location on same line if space allows
    if (isFullScreen) {
        setMediumFont();
        int16_t tempX = leftMargin + 100; // Offset for location
        display.setCursor(tempX, currentY);
        RTCConfigData& config = ConfigManager::getConfig();
        display.print(config.cityName);
    }
    currentY += isFullScreen ? 40 : 35;
    
    // Location (if not already shown)
    if (!isFullScreen) {
        setMediumFont();
        display.setCursor(leftMargin, currentY);
        RTCConfigData& config = ConfigManager::getConfig();
        String city = String(config.cityName);
        if (city.length() > 12 && !isFullScreen) {
            city = city.substring(0, 9) + "...";
        }
        display.print(city);
        currentY += 25;
    }
    
    // Condition
    setSmallFont();
    display.setCursor(leftMargin, currentY);
    String condition = weather.condition;
    if (condition.length() > 15 && !isFullScreen) {
        condition = condition.substring(0, 12) + "...";
    }
    display.print(condition);
    currentY += 20;
    
    // High/Low
    display.setCursor(leftMargin, currentY);
    display.print("High: ");
    display.print(weather.tempMax);
    display.print("  Low: ");
    display.print(weather.tempMin);
    currentY += 20;
    
    // Forecast section
    if (isFullScreen) {
        setSmallFont();
        display.setCursor(leftMargin, currentY);
        display.print("Next Hours:");
        currentY += 18;
        
        int maxForecast = isFullScreen ? 6 : 3;
        for (int i = 0; i < min(maxForecast, weather.forecastCount); i++) {
            const auto& forecast = weather.forecast[i];
            display.setCursor(leftMargin, currentY);
            
            String timeStr = forecast.time.substring(11, 16); // HH:MM
            display.print(timeStr);
            display.print(" ");
            display.print(forecast.temperature);
            display.print(" ");
            display.print(forecast.rainChance);
            display.print("%");
            
            currentY += 16;
            if (currentY > y + h - 20) break;
        }
    }
    
    // Sunrise/Sunset at bottom if space
    if (currentY < y + h - 40 && isFullScreen) {
        currentY = y + h - 25;
        setSmallFont();
        display.setCursor(leftMargin, currentY);
        display.print("☀️↑ ");
        display.print(weather.sunrise);
        display.print("  ☀️↓ ");
        display.print(weather.sunset);
    }
}

void DisplayManager::drawDepartureSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t currentY = y + 25;
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;
    
    bool isFullScreen = (w >= screenWidth * 0.8);
    
    // Title
    if (isFullScreen) {
        setLargeFont();
        display.setCursor(leftMargin, currentY);
        display.print("🚌 Departure Board");
        currentY += 40;
    } else {
        setMediumFont();
        display.setCursor(leftMargin, currentY);
        display.print("🚌 Departures");
        currentY += 30;
    }
    
    // Station name
    setMediumFont();
    display.setCursor(leftMargin, currentY);
    String stopName = departures.stopName;
    int maxNameLength = isFullScreen ? 30 : 20;
    if (stopName.length() > maxNameLength) {
        stopName = stopName.substring(0, maxNameLength - 3) + "...";
    }
    display.print(stopName);
    currentY += 30;
    
    // Column headers
    setSmallFont();
    display.setCursor(leftMargin, currentY);
    if (isFullScreen) {
        display.print("Line    Destination           Time   Track");
    } else {
        display.print("Line  Dest      Time");
    }
    currentY += 18;
    
    // Underline
    display.drawLine(leftMargin, currentY - 5, rightMargin, currentY - 5, GxEPD_BLACK);
    currentY += 8;
    
    // Departures
    int maxDepartures = isFullScreen ? 15 : 10;
    maxDepartures = min(maxDepartures, departures.departureCount);
    
    for (int i = 0; i < maxDepartures; i++) {
        const auto& dep = departures.departures[i];
        
        display.setCursor(leftMargin, currentY);
        
        // Format for available width
        if (isFullScreen) {
            // Full screen format: "Line    Destination           Time   Track"
            String line = dep.line.substring(0, 7);
            while (line.length() < 8) line += " ";
            
            String dest = dep.direction.substring(0, 18);
            while (dest.length() < 19) dest += " ";
            
            String time = dep.rtTime.length() > 0 ? dep.rtTime : dep.time;
            time = time.substring(0, 5);
            while (time.length() < 6) time += " ";
            
            String track = dep.track.substring(0, 3);
            
            display.print(line);
            display.print(dest);
            display.print(time);
            display.print(track);
            
        } else {
            // Half screen format: "Line  Dest      Time"
            String line = dep.line.substring(0, 4);
            while (line.length() < 5) line += " ";
            
            String dest = dep.direction.substring(0, 8);
            while (dest.length() < 9) dest += " ";
            
            String time = dep.rtTime.length() > 0 ? dep.rtTime : dep.time;
            time = time.substring(0, 5);
            
            display.print(line);
            display.print(dest);
            display.print(time);
        }
        
        currentY += 16;
        if (currentY > y + h - 25) break;
    }
    
    // Footer
    if (currentY < y + h - 25) {
        currentY = y + h - 15;
        setSmallFont();
        display.setCursor(leftMargin, currentY);
        display.print("Updated: ");
        display.print(String(millis() / 1000));
        display.print("s ago");
    }
}

void DisplayManager::clearRegion(DisplayRegion region) {
    int16_t x, y, w, h;
    getRegionBounds(region, x, y, w, h);
    
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
    } while (display.nextPage());
}

void DisplayManager::getRegionBounds(DisplayRegion region, int16_t& x, int16_t& y, int16_t& w, int16_t& h) {
    switch (region) {
        case DisplayRegion::FULL_SCREEN:
            x = 0; y = 0; w = screenWidth; h = screenHeight;
            break;
            
        // Landscape-specific regions
        case DisplayRegion::LEFT_HALF:
            x = 0; y = 0; w = halfWidth; h = screenHeight;
            break;
        case DisplayRegion::RIGHT_HALF:
            x = halfWidth; y = 0; w = halfWidth; h = screenHeight;
            break;
            
        // Portrait-specific regions
        case DisplayRegion::UPPER_HALF:
            x = 0; y = 0; w = screenWidth; h = halfHeight;
            break;
        case DisplayRegion::LOWER_HALF:
            x = 0; y = halfHeight; w = screenWidth; h = halfHeight;
            break;
            
        // Semantic regions that adapt to orientation
        case DisplayRegion::WEATHER_AREA:
            if (currentOrientation == DisplayOrientation::LANDSCAPE) {
                // Weather is in LEFT half in landscape
                getRegionBounds(DisplayRegion::LEFT_HALF, x, y, w, h);
            } else {
                // Weather is in UPPER half in portrait
                getRegionBounds(DisplayRegion::UPPER_HALF, x, y, w, h);
            }
            break;
        case DisplayRegion::DEPARTURE_AREA:
            if (currentOrientation == DisplayOrientation::LANDSCAPE) {
                // Departures is in RIGHT half in landscape
                getRegionBounds(DisplayRegion::RIGHT_HALF, x, y, w, h);
            } else {
                // Departures is in LOWER half in portrait
                getRegionBounds(DisplayRegion::LOWER_HALF, x, y, w, h);
            }
            break;
    }
}

void DisplayManager::hibernate() {
    display.hibernate();
    ESP_LOGI(TAG, "Display hibernated");
}

void DisplayManager::setLargeFont() {
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
}

void DisplayManager::setMediumFont() {
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
}

void DisplayManager::setSmallFont() {
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
}
