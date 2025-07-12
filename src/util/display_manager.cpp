#include "util/display_manager.h"
#include <Arduino.h>
#include <esp_log.h>
#include "config/config_manager.h"

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include "config/pins.h"

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;

// U8g2 fonts for German character support
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

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
    
    // Initialize U8g2 fonts for German character support
    u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                 // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);            // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    
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
        u8g2Fonts.setCursor(leftMargin, currentY);
        u8g2Fonts.print("Weather Information");
        currentY += 45;
    } else {
        setMediumFont();
        u8g2Fonts.setCursor(leftMargin, currentY);
        u8g2Fonts.print("Weather");
        currentY += 30;
    }
    
    // Current temperature - make it prominent
    setLargeFont();
    u8g2Fonts.setCursor(leftMargin, currentY);
    u8g2Fonts.print(weather.temperature);
    
    // Location on same line if space allows
    if (isFullScreen) {
        setMediumFont();
        int16_t tempX = leftMargin + 100; // Offset for location
        u8g2Fonts.setCursor(tempX, currentY);
        RTCConfigData& config = ConfigManager::getConfig();
        u8g2Fonts.print(config.cityName); // Now supports German characters!
    }
    currentY += isFullScreen ? 40 : 35;
    
    // Location (if not already shown)
    if (!isFullScreen) {
        setMediumFont();
        u8g2Fonts.setCursor(leftMargin, currentY);
        RTCConfigData& config = ConfigManager::getConfig();
        String city = String(config.cityName);
        if (city.length() > 12 && !isFullScreen) {
            city = city.substring(0, 9) + "...";
        }
        u8g2Fonts.print(city); // Now supports German characters!
        currentY += 25;
    }
    
    // Condition
    setSmallFont();
    u8g2Fonts.setCursor(leftMargin, currentY);
    String condition = weather.condition;
    if (condition.length() > 15 && !isFullScreen) {
        condition = condition.substring(0, 12) + "...";
    }
    u8g2Fonts.print(condition); // Now supports German weather conditions!
    currentY += 20;
    
    // High/Low
    u8g2Fonts.setCursor(leftMargin, currentY);
    u8g2Fonts.print("High: ");
    u8g2Fonts.print(weather.tempMax);
    u8g2Fonts.print("  Low: ");
    u8g2Fonts.print(weather.tempMin);
    currentY += 20;
    
    // Forecast section
    if (isFullScreen) {
        setSmallFont();
        u8g2Fonts.setCursor(leftMargin, currentY);
        u8g2Fonts.print("Next Hours:");
        currentY += 18;
        
        int maxForecast = isFullScreen ? 6 : 3;
        for (int i = 0; i < min(maxForecast, weather.forecastCount); i++) {
            const auto& forecast = weather.forecast[i];
            u8g2Fonts.setCursor(leftMargin, currentY);
            
            String timeStr = forecast.time.substring(11, 16); // HH:MM
            u8g2Fonts.print(timeStr);
            u8g2Fonts.print(" ");
            u8g2Fonts.print(forecast.temperature);
            u8g2Fonts.print(" ");
            u8g2Fonts.print(forecast.rainChance);
            u8g2Fonts.print("%");
            
            currentY += 16;
            if (currentY > y + h - 20) break;
        }
    }
    
    // Sunrise/Sunset at bottom if space
    if (currentY < y + h - 40 && isFullScreen) {
        currentY = y + h - 25;
        setSmallFont();
        u8g2Fonts.setCursor(leftMargin, currentY);
        u8g2Fonts.print("Sunrise: ");
        u8g2Fonts.print(weather.sunrise);
        u8g2Fonts.print("  Sunset: ");
        u8g2Fonts.print(weather.sunset);
    }
}

void DisplayManager::drawDepartureSection(const DepartureData& departures, int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t currentY = y + 25;
    int16_t leftMargin = x + 10;
    int16_t rightMargin = x + w - 10;
    
    bool isFullScreen = (w >= screenWidth * 0.8);
    
    // Station name
    setMediumFont();
    u8g2Fonts.setCursor(leftMargin, currentY);
    RTCConfigData& config = ConfigManager::getConfig();
    String stopName = config.selectedStopName; 
    int maxNameLength = isFullScreen ? 50 : 30;
    if (stopName.length() > maxNameLength) {
        stopName = stopName.substring(0, maxNameLength - 3) + "...";
    }
    u8g2Fonts.print(stopName); // Now supports German station names!
    currentY += 30;
    
    // Column headers
    setSmallFont();
    u8g2Fonts.setCursor(leftMargin, currentY);
    if (isFullScreen) {
        u8g2Fonts.print("Line    Destination           Time   Track");
    } else {
        u8g2Fonts.print("Zeit  Linie  Richtung");
    }
    currentY += 18;
    
    // Underline
    display.drawLine(leftMargin, currentY - 5, rightMargin, currentY - 5, GxEPD_BLACK);
    currentY += 8;
    
    // Departures
    int maxDepartures = isFullScreen ? 20 : 15;
    maxDepartures = min(maxDepartures, departures.departureCount);
    
    for (int i = 0; i < maxDepartures; i++) {
        const auto& dep = departures.departures[i];
        
        u8g2Fonts.setCursor(leftMargin, currentY);
        
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
            
            u8g2Fonts.print(line);
            u8g2Fonts.print(dest); // Now supports German destinations like "Düsseldorf"!
            u8g2Fonts.print(time);
            u8g2Fonts.print(track);
            
        } else {
            // Half screen format: "Time Line Dest" with text wrapping
            String time = dep.rtTime.length() > 0 ? dep.rtTime : dep.time;
            time = time.substring(0, 5);
            while (time.length() < 6) time += " ";

            // Remove "Bus" prefix (case-insensitive)
            String line = dep.line;
            String lineLower = line;
            lineLower.toLowerCase();
            if (lineLower.startsWith("bus ")) {
                line = line.substring(4); // Remove "Bus " prefix
            }

            line = line.substring(0, 4);
            while (line.length() < 5) line += " ";
        
            String dest = dep.direction;
            String destLower = dest;
            destLower.toLowerCase(); 
            // remove destination prefix "Frankfurt (Main) " case insensitive
            if (destLower.startsWith("frankfurt (main) ")) {
                dest = dep.direction.substring(17); // Remove "Frankfurt (Main) "
            } else {
                dest = dep.direction;
            }
            
            // Print time and line on first line
            u8g2Fonts.print(time);
            u8g2Fonts.print(line);
            
            // Handle destination with text wrapping
            int maxDestWidth = 20; // Maximum characters per line for destination
            if (dest.length() <= maxDestWidth) {
                // Short destination - print on same line
                u8g2Fonts.print(dest); // Now supports German destinations like "München"!
            } else {
                // Long destination - wrap to next line
                String firstPart = dest.substring(0, maxDestWidth - 3) + "...";
                u8g2Fonts.print(firstPart); // Now supports German destinations!
                
                // Move to next line for continuation (optional)
                currentY += 16;
                if (currentY <= y + h - 40) { // Check if we have space for another line
                    u8g2Fonts.setCursor(leftMargin + 60, currentY); // Indent continuation
                    String secondPart = dest.substring(maxDestWidth - 3);
                    if (secondPart.length() > maxDestWidth) {
                        secondPart = secondPart.substring(0, maxDestWidth - 3) + "...";
                    }
                    u8g2Fonts.print(secondPart); // Now supports German destinations!
                }
            }
        }
        
        // Adjust line spacing - more space if text might be wrapped
        if (isFullScreen || dep.direction.length() <= 20) {
            currentY += 16; // Normal spacing
        } else {
            currentY += 20; // Extra spacing for wrapped text
        }
        
        if (currentY > y + h - 25) break;
    }
    
    // Footer
    if (currentY < y + h - 25) {
        currentY = y + h - 15;
        setSmallFont();
        u8g2Fonts.setCursor(leftMargin, currentY);

        // Get current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        char timeStr[16];
        // German time format: "HH:MM Uhr"
        strftime(timeStr, sizeof(timeStr), "%H:%M Uhr", &timeinfo);

        u8g2Fonts.print("Aktualisiert: ");
        u8g2Fonts.print(timeStr);
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
    u8g2Fonts.setFont(u8g2_font_helvB18_tf); // 18pt Helvetica Bold with German characters
}

void DisplayManager::setMediumFont() {
    u8g2Fonts.setFont(u8g2_font_helvB12_tf); // 12pt Helvetica Bold with German characters
}

void DisplayManager::setSmallFont() {
    u8g2Fonts.setFont(u8g2_font_helvB08_tf); // 8pt Helvetica Bold with German characters
}

// Helper function for wrapping long text
void DisplayManager::printWrappedText(const String& text, int16_t x, int16_t& y, int16_t maxWidth, int16_t maxChars, int16_t lineHeight) {
    if (text.length() <= maxChars) {
        // Text fits on one line
        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(text); // Now supports German characters!
    } else {
        // Text needs wrapping
        String firstLine = text.substring(0, maxChars - 3) + "...";
        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(firstLine); // Now supports German characters!
        
        // Check if we have space for a second line
        if (y + lineHeight < screenHeight - 25) {
            y += lineHeight;
            u8g2Fonts.setCursor(x + 20, y); // Slight indent for continuation
            String secondLine = text.substring(maxChars - 3);
            if (secondLine.length() > maxChars) {
                secondLine = secondLine.substring(0, maxChars - 3) + "...";
            }
            u8g2Fonts.print(secondLine); // Now supports German characters!
        }
    }
}
