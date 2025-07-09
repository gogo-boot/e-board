#include "util/departure_print.h"
#include <Arduino.h>
#include <esp_log.h>

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

static const char* TAG = "DEPARTURE";

void printDepartInfo(const DepartureData& depart) {
    ESP_LOGI(TAG, "--- DepartureInfo ---");
    ESP_LOGI(TAG, "Stop: %s (%s)", depart.stopName.c_str(), depart.stopId.c_str());
    ESP_LOGI(TAG, "Departure count: %d", depart.departureCount);
    
    for (int i = 0; i < depart.departureCount && i < 10; ++i) {
        const auto &dep = depart.departures[i];
        ESP_LOGI(TAG, "Departure %d | Line: %s | Direction: %s | Time: %s | RT Time: %s | Track: %s | Category: %s",
                 i + 1,
                 dep.line.c_str(),
                 dep.direction.c_str(),
                 dep.time.c_str(),
                 dep.rtTime.c_str(),
                 dep.track.c_str(),
                 dep.category.c_str());
    }
    ESP_LOGI(TAG, "--- End DepartureInfo ---");
}

void displayDepartInfo(const DepartureData& depart) {
    ESP_LOGI(TAG, "Displaying departure info on RIGHT half of e-paper display");
    
    // Right half dimensions: 400-800px width, full height
    const int16_t RIGHT_WIDTH = 400;
    const int16_t RIGHT_X_START = 400;
    const int16_t RIGHT_X_END = 800;
    
    // Use full window but only draw on right half
    display.setFullWindow();
    display.firstPage();
    
    do {
        // Clear only the right half (don't touch left half)
        display.fillRect(RIGHT_X_START, 0, RIGHT_WIDTH, display.height(), GxEPD_WHITE);
        
        int16_t y = 30; // Starting Y position
        int16_t x = RIGHT_X_START + 10; // Right half left margin
        
        // Title
        display.setFont(&FreeMonoBold18pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(x, y);
        display.print("Departures");
        y += 40;
        
        // Stop name (truncated to fit right half)
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(x, y);
        String stopName = depart.stopName;
        if (stopName.length() > 25) stopName = stopName.substring(0, 22) + "...";
        display.print("Stop: ");
        display.print(stopName);
        y += 35;
        
        // Column headers (adjusted for right half width)
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(x, y);
        display.print("Line  Time  Track Dir");
        y += 25;
        
        // Draw separator line (only in right half)
        display.drawLine(x, y, RIGHT_X_END - 10, y, GxEPD_BLACK);
        y += 15;
        
        // Show departures (adjusted for right half)
        int maxDepartures = min(depart.departureCount, 12); // More entries possible in right half
        for (int i = 0; i < maxDepartures; ++i) {
            const auto &dep = depart.departures[i];
            
            // Format strings for right half (shorter widths)
            String lineStr = dep.line;
            if (lineStr.length() > 4) lineStr = lineStr.substring(0, 4);
            
            // Use real-time if available, otherwise scheduled time
            String timeStr = dep.rtTime.length() > 0 ? dep.rtTime : dep.time;
            if (timeStr.length() > 5) timeStr = timeStr.substring(0, 5);
            
            // Format track (max 3 chars for right half)
            String trackStr = dep.track;
            if (trackStr.length() > 3) trackStr = trackStr.substring(0, 3);
            
            // Format direction (shorter for right half)
            String dirStr = dep.direction;
            if (dirStr.length() > 15) dirStr = dirStr.substring(0, 12) + "...";
            
            // Display the departure info
            display.setCursor(x, y);
            
            // Line (4 chars width)
            display.print(lineStr);
            for (int pad = lineStr.length(); pad < 6; pad++) display.print(" ");
            
            // Time (5 chars width)
            display.print(timeStr);
            for (int pad = timeStr.length(); pad < 6; pad++) display.print(" ");
            
            // Track (3 chars width)
            display.print(trackStr);
            for (int pad = trackStr.length(); pad < 5; pad++) display.print(" ");
            
            // Direction
            display.print(dirStr);
            
            y += 18; // Tighter spacing for more entries
            
            // Check if we're running out of vertical space
            if (y > display.height() - 30) {
                ESP_LOGI(TAG, "Right half height limit reached");
                break;
            }
        }
        
        // Footer with update info (in right half)
        if (y < display.height() - 25) {
            y = display.height() - 20;
            display.setFont(&FreeMonoBold9pt7b);
            display.setCursor(x, y);
            display.print("Updated: ");
            display.print(String(millis() / 1000));
            display.print("s");
        }
        
        // Draw vertical separator line between halves
        display.drawLine(RIGHT_X_START, 0, RIGHT_X_START, display.height(), GxEPD_BLACK);
        
    } while (display.nextPage());
    
    ESP_LOGI(TAG, "Departure info displayed on RIGHT half of e-paper");
}
