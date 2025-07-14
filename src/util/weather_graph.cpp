#include "util/weather_graph.h"
#include "util/display_manager.h"
#include <esp_log.h>
#include <math.h>

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>
#include <U8g2_for_Adafruit_GFX.h>

// External display instances
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

static const char* TAG = "WEATHER_GRAPH";

void WeatherGraph::drawTemperatureAndRainGraph(const WeatherInfo& weather, 
                                              int16_t x, int16_t y, 
                                              int16_t w, int16_t h) {
    if (weather.hourlyForecastCount == 0) {
        ESP_LOGW(TAG, "No hourly weather data available for graph");
        return;
    }
    
    ESP_LOGI(TAG, "Drawing weather graph at (%d,%d) size %dx%d", x, y, w, h);
    
    // Adaptive margins based on graph size
    int16_t marginLeft = (h < 120) ? 25 : MARGIN_LEFT;      // Smaller margins for compact mode
    int16_t marginRight = (h < 120) ? 25 : MARGIN_RIGHT;
    int16_t marginTop = (h < 120) ? 10 : MARGIN_TOP;
    int16_t marginBottom = (h < 120) ? 20 : MARGIN_BOTTOM;
    
    // Calculate graph area (removing margins for axes labels)
    int16_t graphX = x + marginLeft;
    int16_t graphY = y + marginTop;
    int16_t graphW = w - marginLeft - marginRight;
    int16_t graphH = h - marginTop - marginBottom;
    
    // Find actual temperature range from data
    float actualMin = 100.0f, actualMax = -100.0f;
    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);
    
    for (int i = 0; i < dataPoints; i++) {
        float temp = parseTemperature(weather.hourlyForecast[i].temperature);
        actualMin = min(actualMin, temp);
        actualMax = max(actualMax, temp);
    }
    
    // Calculate dynamic temperature range using your specified logic
    float dynamicMin = calculateDynamicMinTemp(actualMin);
    float dynamicMax = calculateDynamicMaxTemp(actualMax);
    
    ESP_LOGI(TAG, "Temperature range: actual %.1f-%.1f°C, dynamic %.1f-%.1f°C", 
             actualMin, actualMax, dynamicMin, dynamicMax);
    
    // Draw graph components
    drawGraphFrame(graphX, graphY, graphW, graphH);
    drawTemperatureAxis(x, graphY, marginLeft, graphH, dynamicMin, dynamicMax);
    drawRainAxis(x + w - marginRight, graphY, marginRight, graphH);
    drawTimeAxis(graphX, y + h - marginBottom, graphW, marginBottom);
    
    // Draw data (rain bars first, then temperature line on top)
    drawRainBars(weather, graphX, graphY, graphW, graphH);
    drawTemperatureLine(weather, graphX, graphY, graphW, graphH, dynamicMin, dynamicMax);
    
    ESP_LOGI(TAG, "Weather graph completed with %d data points", dataPoints);
}

void WeatherGraph::drawGraphFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Draw main graph border
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    
    // Draw horizontal grid lines (every 25% of height)
    for (int i = 1; i < 4; i++) {
        int16_t gridY = y + (h * i) / 4;
        // Dotted line for grid
        for (int16_t dotX = x + 5; dotX < x + w - 5; dotX += 6) {
            display.drawPixel(dotX, gridY, GxEPD_BLACK);
        }
    }
    
    // Draw vertical grid lines (every 3 hours)
    for (int i = 1; i < 4; i++) {
        int16_t gridX = x + (w * i) / 4;
        // Dotted line for grid
        for (int16_t dotY = y + 5; dotY < y + h - 5; dotY += 6) {
            display.drawPixel(gridX, dotY, GxEPD_BLACK);
        }
    }
}

void WeatherGraph::drawTemperatureAxis(int16_t x, int16_t y, int16_t w, int16_t h, 
                                      float minTemp, float maxTemp) {
    DisplayManager::setSmallFont();
    
    // Calculate temperature steps (reduce labels for compact mode)
    int labelCount = (w < 30) ? 3 : 5; // 3 labels for compact, 5 for full
    float tempStep = (maxTemp - minTemp) / (labelCount - 1);
    
    for (int i = 0; i < labelCount; i++) {
        float temp = minTemp + (tempStep * i);
        int16_t labelY = y + h - (h * i / (labelCount - 1));
        
        // Format temperature (whole numbers for cleaner display)
        String tempLabel = String((int)round(temp)) + "°";
        
        // Right-align the temperature labels
        int16_t textWidth = DisplayManager::getTextWidth(tempLabel);
        u8g2.setCursor(x + w - textWidth - 3, labelY + 4);
        u8g2.print(tempLabel);
    }
    
    // Temperature axis label (skip for very compact mode)
    if (w >= 30) {
        DisplayManager::setSmallFont();
        u8g2.setCursor(x + 2, y - 5);
        u8g2.print("°C");
    }
}

void WeatherGraph::drawRainAxis(int16_t x, int16_t y, int16_t w, int16_t h) {
    DisplayManager::setSmallFont();
    
    // Rain percentage labels (reduce for compact mode)
    int labelCount = (w < 30) ? 3 : 5; // 3 labels for compact (0%, 50%, 100%), 5 for full
    
    for (int i = 0; i < labelCount; i++) {
        int rainPercent = (i * 100) / (labelCount - 1); // Distribute evenly
        int16_t labelY = y + h - (h * i / (labelCount - 1));
        
        String rainLabel = String(rainPercent) + "%";
        u8g2.setCursor(x + 3, labelY + 4);
        u8g2.print(rainLabel);
    }
    
    // Rain axis label (skip for very compact mode)
    if (w >= 30) {
        u8g2.setCursor(x + 3, y - 5);
        u8g2.print("%");
    }
}

void WeatherGraph::drawTimeAxis(int16_t x, int16_t y, int16_t w, int16_t h) {
    DisplayManager::setSmallFont();
    
    // Adaptive time labels based on available width
    int labelStep = (w < 200) ? 6 : 3; // Every 6 hours for compact, every 3 hours for full
    int maxLabels = HOURS_TO_SHOW / labelStep + 1;
    
    for (int i = 0; i < maxLabels; i++) {
        int hourIndex = i * labelStep;
        if (hourIndex <= HOURS_TO_SHOW) {
            String timeLabel = String(hourIndex) + "h";
            
            int16_t labelX = x + (w * i * labelStep) / HOURS_TO_SHOW;
            int16_t textWidth = DisplayManager::getTextWidth(timeLabel);
            
            // Make sure label fits within bounds
            if (labelX + textWidth/2 <= x + w) {
                u8g2.setCursor(labelX - textWidth / 2, y + 15);
                u8g2.print(timeLabel);
            }
        }
    }
}

void WeatherGraph::drawTemperatureLine(const WeatherInfo& weather, 
                                     int16_t graphX, int16_t graphY, 
                                     int16_t graphW, int16_t graphH,
                                     float minTemp, float maxTemp) {
    // Get the number of data points to draw (limited to HOURS_TO_SHOW = 12)
    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);
    
    // Need at least 2 points to draw a line
    if (dataPoints < 2) return;
    
    // === STEP 1: Draw temperature line segments connecting each hour ===
    for (int i = 0; i < dataPoints - 1; i++) {
        // Parse temperature values for current hour (i) and next hour (i+1)
        float temp1 = parseTemperature(weather.hourlyForecast[i].temperature);
        float temp2 = parseTemperature(weather.hourlyForecast[i + 1].temperature);
        
        // Convert hour index to X pixel position on the graph
        // Maps hour 0 to graphX, hour 11 to graphX + graphW
        int16_t x1 = mapToPixel(i, 0, HOURS_TO_SHOW - 1, graphX, graphX + graphW);
        int16_t x2 = mapToPixel(i + 1, 0, HOURS_TO_SHOW - 1, graphX, graphX + graphW);
        
        // Convert temperature to Y pixel position on the graph
        // Maps minTemp to bottom (graphY + graphH), maxTemp to top (graphY)
        int16_t y1 = mapToPixel(temp1, minTemp, maxTemp, graphY + graphH, graphY);
        int16_t y2 = mapToPixel(temp2, minTemp, maxTemp, graphY + graphH, graphY);
        
        // Draw thick line for e-paper visibility (3 parallel lines)
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);           // Main line
    }
    
    // === STEP 2: Draw temperature data points (circles) at each hour ===
    for (int i = 0; i < dataPoints; i++) {
        // Parse temperature for this hour
        float temp = parseTemperature(weather.hourlyForecast[i].temperature);
        
        // Convert hour index and temperature to pixel coordinates
        int16_t x = mapToPixel(i, 0, HOURS_TO_SHOW - 1, graphX, graphX + graphW);
        int16_t y = mapToPixel(temp, minTemp, maxTemp, graphY + graphH, graphY);
        
        // Draw filled circle (radius 3 pixels) at each data point
        display.fillCircle(x, y, 3, GxEPD_BLACK);
        
        // === STEP 3: Optional temperature value labels (only in full-size mode) ===
        // Show temperature values near specific points to avoid clutter
        // Only show for: first hour (0), middle hour (6), and last hour (11)
        if (graphH > 150 && (i == 0 || i == dataPoints/2 || i == dataPoints - 1)) {
            DisplayManager::setSmallFont();
            
            // Format temperature as whole number with degree symbol (e.g., "17°")
            String tempStr = String((int)round(temp)) + "°";
            int16_t textWidth = DisplayManager::getTextWidth(tempStr);
            
            // Position text above or below the point to avoid overlap with the line
            // If point is in upper half of graph, put text below (y + 15)
            // If point is in lower half of graph, put text above (y - 5)
            int16_t textY = (y < graphY + graphH/2) ? y + 15 : y - 5;
            
            // Center the text horizontally on the data point
            u8g2.setCursor(x - textWidth/2, textY);
            u8g2.print(tempStr);
        }
    }
}

void WeatherGraph::drawRainBars(const WeatherInfo& weather,
                               int16_t graphX, int16_t graphY, 
                               int16_t graphW, int16_t graphH) {
    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);
    int16_t barWidth = graphW / HOURS_TO_SHOW;
    
    for (int i = 0; i < dataPoints; i++) {
        int rainChance = parseRainChance(weather.hourlyForecast[i].rainChance);
        
        if (rainChance > 0) {
            int16_t barX = graphX + (i * graphW) / HOURS_TO_SHOW;
            int16_t barH = (graphH * rainChance) / 100;
            int16_t barY = graphY + graphH - barH;
            
            // Draw rain bar with a pattern to distinguish from temperature line
            // Use diagonal stripes pattern
            for (int16_t stripe = 0; stripe < barH; stripe += 3) {
                int16_t stripeY = barY + stripe;
                for (int16_t x = barX; x < barX + barWidth - 2; x += 4) {
                    display.drawPixel(x + (stripe % 2), stripeY, GxEPD_BLACK);
                }
            }
            
            // Draw bar outline for clarity
            display.drawRect(barX, barY, barWidth - 1, barH, GxEPD_BLACK);
        }
    }
}

// Utility function implementations
float WeatherGraph::parseTemperature(const String& tempStr) {
    return tempStr.toFloat();
}

int WeatherGraph::parseRainChance(const String& rainStr) {
    return rainStr.toInt();
}

float WeatherGraph::calculateDynamicMinTemp(float actualMin) {
    // Your logic: divide by 5 (integer division), then multiply by 5
    // Example: 17°C -> 17/5 = 3 -> 3*5 = 15°C
    int tempDiv = (int)actualMin / 5;
    return (float)(tempDiv * 5);
}

float WeatherGraph::calculateDynamicMaxTemp(float actualMax) {
    // Your logic: divide by 5 (integer division), add 1, then multiply by 5
    // Example: 23°C -> 23/5 = 4 -> (4+1)*5 = 25°C
    int tempDiv = (int)actualMax / 5;
    return (float)((tempDiv + 1) * 5);
}

int16_t WeatherGraph::mapToPixel(float value, float minVal, float maxVal, 
                                int16_t minPixel, int16_t maxPixel) {
    if (maxVal == minVal) return minPixel;
    return minPixel + (int16_t)((value - minVal) * (maxPixel - minPixel) / (maxVal - minVal));
}

String WeatherGraph::formatHourFromTime(const String& timeStr) {
    // Extract hour from ISO time string (YYYY-MM-DDTHH:MM:SS)
    if (timeStr.length() >= 16) {
        return timeStr.substring(11, 13) + "h";
    }
    return "??h";
}
