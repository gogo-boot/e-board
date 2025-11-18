#include "display/weather_graph.h"
#include "display/text_utils.h"
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
    int16_t marginLeft = (h < 120) ? 25 : MARGIN_LEFT;
    int16_t marginRight = (h < 120) ? 25 : MARGIN_RIGHT;
    int16_t marginTop = (h < 120) ? 10 : MARGIN_TOP;
    int16_t marginBottom = (h < 120) ? 20 : MARGIN_BOTTOM;
    int16_t marginLegend = (h < 120) ? 20 : LEGEND_MARGIN;

    // Calculate graph area (removing margins for axes labels)
    int16_t graphX = x + marginLeft;
    int16_t graphY = y + marginTop;
    int16_t graphW = w - marginLeft - marginRight;
    int16_t graphH = h - marginTop - marginBottom - marginLegend; // Reserve space for legend

    // Find actual temperature range from data
    float actualMin = 100.0f, actualMax = -100.0f;
    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);

    for (int i = 0; i < dataPoints; i++) {
        float temp = weather.hourlyForecast[i].temperature;
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
    drawTimeAxis(graphX, y + h - marginBottom - marginLegend, graphW, marginBottom, weather);
    // <-- Add weather parameter
    drawGraphLegend(x, y + h - marginLegend, w, marginLegend);

    // Draw data layers (order matters for visibility)
    drawRainBars(weather, graphX, graphY, graphW, graphH); // Background: Rain bars
    drawHumidityLine(weather, graphX, graphY, graphW, graphH); // Middle: Humidity dotted line
    drawTemperatureLine(weather, graphX, graphY, graphW, graphH, dynamicMin, dynamicMax);
    // Foreground: Temperature solid line

    ESP_LOGI(TAG, "Weather graph completed with %d data points", dataPoints);
}

void WeatherGraph::drawGraphLegend(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Draw the legend horizontally (side by side), using available width

    TextUtils::setFont8px_margin10px();

    int itemCount = 3;
    int16_t spacingX = w / itemCount;
    int16_t lineLen = min(32, spacingX - 40); // leave space for label text

    int16_t legendY = y + h / 2 - 6; // vertically center the legend items

    // 1. Temperature (solid line)
    int16_t legendX = x;
    display.drawLine(legendX, legendY + 6, legendX + lineLen, legendY + 6, GxEPD_BLACK); // Draw solid line
    u8g2.setCursor(legendX + lineLen + 8, legendY + 10);
    u8g2.print("Temperatur");

    // 2. Humidity (dotted line)
    legendX += spacingX;
    u8g2.setCursor(legendX + lineLen + 8, legendY + 10);
    u8g2.print("Luftfeuchte");
    // Draw dotted line
    for (int i = 0; i < lineLen; ++i) {
        if ((i / 3) % 3 == 0) display.drawPixel(legendX + i, legendY + 6, GxEPD_BLACK);
    }

    // 3. Rain (crosshatch bar)
    legendX += spacingX;
    u8g2.setCursor(legendX + lineLen + 8, legendY + 10);
    u8g2.print("Regen Chance");
    // Draw crosshatch bar
    int16_t barTop = legendY + 2;
    int16_t barHeight = min(8, h - 4); // fit in legend area
    // Horizontal lines
    for (int yb = barTop; yb < barTop + barHeight; yb += 3) {
        for (int xb = legendX; xb < legendX + lineLen; xb += 2) {
            display.drawPixel(xb, yb, GxEPD_BLACK);
        }
    }
    // Vertical lines
    for (int xb = legendX; xb < legendX + lineLen; xb += 4) {
        for (int yb = barTop; yb < barTop + barHeight; yb += 2) {
            display.drawPixel(xb, yb, GxEPD_BLACK);
        }
    }
}

void WeatherGraph::drawGraphFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Draw only top and bottom borders (remove left and right Y-axis lines)
    display.drawLine(x, y + h, x + w, y + h, GxEPD_BLACK); // Bottom border

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
    TextUtils::setFont10px_margin12px(); // Small font for axis labels

    // Calculate temperature steps (reduce labels for compact mode)
    int labelCount = (w < 30) ? 3 : 5; // 3 labels for compact, 5 for full

    // Force 2.5°C steps for clean labeling
    float tempRange = maxTemp - minTemp;
    float tempStep = tempRange / (labelCount - 1);

    // Round step to nearest 0.5 for cleaner labels (2.5, 3.0, etc.)
    tempStep = round(tempStep * 2.0f) / 2.0f;

    ESP_LOGI(TAG, "Temperature axis: min=%.1f, max=%.1f, step=%.1f", minTemp, maxTemp, tempStep);

    for (int i = 0; i < labelCount; i++) {
        float temp = minTemp + (tempStep * i);
        int16_t labelY = y + h - (h * i / (labelCount - 1));

        // Format temperature with 1 decimal place to show .5 values
        String tempLabel;
        if (temp == (int)temp) {
            tempLabel = String((int)temp) + "°"; // Show "20°" for whole numbers
        } else {
            tempLabel = String(temp, 1) + "°"; // Show "17.5°" for half numbers
        }

        // Right-align the temperature labels
        int16_t textWidth = TextUtils::getTextWidth(tempLabel);
        u8g2.setCursor(x + w - textWidth - 3, labelY + 4);
        u8g2.print(tempLabel);
    }

    // Temperature axis label (skip for very compact mode)
    if (w >= 30) {
        // Place "Temperatur" above the Y axis, left-aligned
        int tempLabelX = x; // 10px left of axis (adjust as needed)
        int tempLabelY = y - 25; // 5px above the graph area (adjust as needed)
        TextUtils::printTextAtWithMargin(tempLabelX, tempLabelY, "Temperatur");
    }
}

void WeatherGraph::drawRainAxis(int16_t x, int16_t y, int16_t w, int16_t h) {
    TextUtils::setFont10px_margin12px(); // Small font for rain axis

    // Rain percentage labels (reduce for compact mode)
    int labelCount = (w < 30) ? 3 : 5; // 3 labels for compact (0%, 50%, 100%), 5 for full

    for (int i = 0; i < labelCount; i++) {
        int rainPercent = (i * 100) / (labelCount - 1); // Distribute evenly
        int16_t labelY = y + h - (h * i / (labelCount - 1));

        String rainLabel = String(rainPercent) + "%";
        u8g2.setCursor(x + 3, labelY + 4);
        u8g2.print(rainLabel);
    }
}

void WeatherGraph::drawTimeAxis(int16_t x, int16_t y, int16_t w, int16_t h, const WeatherInfo& weather) {
    TextUtils::setFont10px_margin12px(); // Small font for time axis

    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);
    if (dataPoints < 2) return;

    // Dynamically choose label count: about one every 3 points, but always at least 2, at most dataPoints
    int labelCount = constrain((dataPoints + 2) / 3, 2, dataPoints);
    // If you want at least 5, use: int labelCount = constrain((dataPoints + 2) / 3, 5, dataPoints);

    for (int l = 0; l < labelCount; l++) {
        // Evenly spaced indices: 0 ... dataPoints-1
        int i = (l * (dataPoints - 1)) / (labelCount - 1);

        String timeStr = (i < weather.hourlyForecastCount) ? weather.hourlyForecast[i].time : "";
        String actualTime;
        if (timeStr.length() >= 16) {
            actualTime = timeStr.substring(11, 16); // "HH:MM"
        } else {
            actualTime = String(i) + "h";
        }

        int16_t labelX = x + (w * i) / (dataPoints - 1);
        int16_t textWidth = TextUtils::getTextWidth(actualTime);

        u8g2.setCursor(labelX - textWidth / 2, y + 20);
        u8g2.print(actualTime);
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

    // === STEP 1: Draw smooth temperature curve through all points ===
    // Create arrays to store all temperature points for smooth curve calculation
    int16_t tempX[HOURS_TO_SHOW];
    int16_t tempY[HOURS_TO_SHOW];

    // Calculate all point positions first
    for (int i = 0; i < dataPoints; i++) {
        float temp = weather.hourlyForecast[i].temperature;
        tempX[i] = mapToPixel(i, 0, HOURS_TO_SHOW - 1, graphX, graphX + graphW);
        tempY[i] = mapToPixel(temp, minTemp, maxTemp, graphY + graphH, graphY);
    }

    // Draw smooth curve using Catmull-Rom spline interpolation
    // This creates smooth curves that pass through all data points
    for (int i = 0; i < dataPoints - 1; i++) {
        // Get control points for smooth curve calculation
        // Use previous and next points to calculate smooth tangents
        int16_t p0x = (i > 0) ? tempX[i - 1] : tempX[i];
        int16_t p0y = (i > 0) ? tempY[i - 1] : tempY[i];
        int16_t p1x = tempX[i];
        int16_t p1y = tempY[i];
        int16_t p2x = tempX[i + 1];
        int16_t p2y = tempY[i + 1];
        int16_t p3x = (i < dataPoints - 2) ? tempX[i + 2] : tempX[i + 1];
        int16_t p3y = (i < dataPoints - 2) ? tempY[i + 2] : tempY[i + 1];

        // Draw smooth curve between p1 and p2 using many small segments
        int smoothSteps = 16; // Higher number = smoother curve

        for (int step = 0; step < smoothSteps; step++) {
            // Calculate interpolation parameter (0.0 to 1.0)
            float t1 = (float)step / smoothSteps;
            float t2 = (float)(step + 1) / smoothSteps;

            // Catmull-Rom spline calculation for smooth curves
            // This creates smooth curves that pass exactly through data points
            auto catmullRom = [](float t, int16_t p0, int16_t p1, int16_t p2, int16_t p3) -> int16_t {
                float t2 = t * t;
                float t3 = t2 * t;

                float result = 0.5f * (
                    (2.0f * p1) +
                    (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                );

                return (int16_t)result;
            };

            // Calculate smooth curve points
            int16_t curve_x1 = catmullRom(t1, p0x, p1x, p2x, p3x);
            int16_t curve_y1 = catmullRom(t1, p0y, p1y, p2y, p3y);
            int16_t curve_x2 = catmullRom(t2, p0x, p1x, p2x, p3x);
            int16_t curve_y2 = catmullRom(t2, p0y, p1y, p2y, p3y);

            // Draw the smooth curve segment
            display.drawLine(curve_x1, curve_y1, curve_x2, curve_y2, GxEPD_BLACK);
        }
    }
}

void WeatherGraph::drawRainBars(const WeatherInfo& weather,
                                int16_t graphX, int16_t graphY,
                                int16_t graphW, int16_t graphH) {
    // Get the number of data points to draw (limited to HOURS_TO_SHOW = 12)
    int dataPoints = min(HOURS_TO_SHOW_BAR, weather.hourlyForecastCount);
    int16_t barWidth = graphW / HOURS_TO_SHOW_BAR;

    for (int i = 0; i < dataPoints; i++) {
        int rainChance = weather.hourlyForecast[i].rainChance;

        if (rainChance > 0) {
            int16_t barX = graphX + (i * graphW) / HOURS_TO_SHOW_BAR;
            int16_t barH = (graphH * rainChance) / 100;
            int16_t barY = graphY + graphH - barH;

            // Crosshatch pattern for rain bars
            for (int16_t y = barY; y < barY + barH; y += 4) {
                // Horizontal lines
                for (int16_t x = barX + 1; x < barX + barWidth - 1; x += 2) {
                    display.drawPixel(x, y, GxEPD_BLACK);
                }
            }

            for (int16_t x = barX + 1; x < barX + barWidth - 1; x += 4) {
                // Vertical lines
                for (int16_t y = barY; y < barY + barH; y += 2) {
                    display.drawPixel(x, y, GxEPD_BLACK);
                }
            }

            // Add outline for clear definition (without bottom line)
            // display.drawLine(barX, barY, barX, barY + barH - 1, GxEPD_BLACK);                    // Left side
            // display.drawLine(barX, barY, barX + barWidth - 1, barY, GxEPD_BLACK);                // Top side
            // display.drawLine(barX + barWidth - 1, barY, barX + barWidth - 1, barY + barH - 1, GxEPD_BLACK); // Right side
        }
    }
}

void WeatherGraph::drawHumidityLine(const WeatherInfo& weather,
                                    int16_t graphX, int16_t graphY,
                                    int16_t graphW, int16_t graphH) {
    // Get the number of data points to draw (limited to HOURS_TO_SHOW = 12)
    int dataPoints = min(HOURS_TO_SHOW, weather.hourlyForecastCount);

    // Need at least 2 points to draw a line
    if (dataPoints < 2) return;

    ESP_LOGI(TAG, "Drawing humidity line with %d data points", dataPoints);

    // Humidity range is always 0-100%
    float minHumidity = 0.0f;
    float maxHumidity = 100.0f;

    // === STEP 1: Draw smooth humidity curve through all points ===
    // Create arrays to store all humidity points for smooth curve calculation
    int16_t humidityX[HOURS_TO_SHOW];
    int16_t humidityY[HOURS_TO_SHOW];

    // Calculate all point positions first
    for (int i = 0; i < dataPoints; i++) {
        float humidity = weather.hourlyForecast[i].humidity;
        humidityX[i] = mapToPixel(i, 0, HOURS_TO_SHOW - 1, graphX, graphX + graphW);
        humidityY[i] = mapToPixel(humidity, minHumidity, maxHumidity, graphY + graphH, graphY);

        ESP_LOGD(TAG, "Humidity %d: %.1f%% at pixel (%d, %d)", i, humidity, humidityX[i], humidityY[i]);
    }

    // Draw smooth dotted curve using Catmull-Rom spline interpolation
    // This creates smooth curves that pass through all data points
    for (int i = 0; i < dataPoints - 1; i++) {
        // Get control points for smooth curve calculation
        // Use previous and next points to calculate smooth tangents
        int16_t p0x = (i > 0) ? humidityX[i - 1] : humidityX[i];
        int16_t p0y = (i > 0) ? humidityY[i - 1] : humidityY[i];
        int16_t p1x = humidityX[i];
        int16_t p1y = humidityY[i];
        int16_t p2x = humidityX[i + 1];
        int16_t p2y = humidityY[i + 1];
        int16_t p3x = (i < dataPoints - 2) ? humidityX[i + 2] : humidityX[i + 1];
        int16_t p3y = (i < dataPoints - 2) ? humidityY[i + 2] : humidityY[i + 1];

        // Draw smooth dotted curve between p1 and p2 using many small segments
        int smoothSteps = 3; // Higher number = smoother curve

        for (int step = 0; step < smoothSteps; step++) {
            // Calculate interpolation parameter (0.0 to 1.0)
            float t1 = (float)step / smoothSteps;
            float t2 = (float)(step + 1) / smoothSteps;

            // Catmull-Rom spline calculation for smooth curves
            // This creates smooth curves that pass exactly through data points
            auto catmullRom = [](float t, int16_t p0, int16_t p1, int16_t p2, int16_t p3) -> int16_t {
                float t2 = t * t;
                float t3 = t2 * t;

                float result = 0.5f * (
                    (2.0f * p1) +
                    (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                );

                return (int16_t)result;
            };

            // Calculate smooth curve points
            int16_t curve_x1 = catmullRom(t1, p0x, p1x, p2x, p3x);
            int16_t curve_y1 = catmullRom(t1, p0y, p1y, p2y, p3y);
            int16_t curve_x2 = catmullRom(t2, p0x, p1x, p2x, p3x);
            int16_t curve_y2 = catmullRom(t2, p0y, p1y, p2y, p3y);

            // Draw the smooth dotted curve segment
            drawDottedLine(curve_x1, curve_y1, curve_x2, curve_y2);
        }
    }
}

void WeatherGraph::drawDottedLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    // Calculate line length and direction using Bresenham's algorithm
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    int16_t x = x1, y = y1;
    int dotCounter = 0;

    // Use shorter dots and gaps for clear dotted pattern
    const int dotLength = 3; // 2 pixels on
    const int gapLength = 4; // 3 pixels off

    while (true) {
        // Draw pixel only during "on" phase of dot pattern
        if ((dotCounter % (dotLength + gapLength)) < dotLength) {
            display.drawPixel(x, y, GxEPD_BLACK);
        }

        // Check if we've reached the end point
        if (x == x2 && y == y2) break;

        // Bresenham's line algorithm for smooth dotted line
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }

        dotCounter++;
    }
}

// Utility function implementations
float WeatherGraph::calculateDynamicMinTemp(float actualMin) {
    // Use floor() to always round down to nearest 5°C
    // Examples:
    //   17°C  -> floor(17/5) = floor(3.4) = 3  -> 3*5 = 15°C
    //   -3°C  -> floor(-3/5) = floor(-0.6) = -1 -> -1*5 = -5°C
    //   -7°C  -> floor(-7/5) = floor(-1.4) = -2 -> -2*5 = -10°C
    return floor(actualMin / 5.0f) * 5.0f;
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
