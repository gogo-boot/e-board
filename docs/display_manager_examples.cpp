/*
 * Display Manager Usage Examples
 * 
 * This file demonstrates how to use the new DisplayManager
 * with different modes and orientations.
 */

#include "display/display_manager.h"
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"

void exampleUsage() {
    // Sample data (you would get this from your APIs)
    WeatherInfo weather;
    DepartureData departures;
    
    // === PORTRAIT MODE EXAMPLES ===
    
    // 1. Half and Half Mode - Portrait (Weather upper, Departures lower)
    DisplayManager::init(DisplayOrientation::PORTRAIT);
    DisplayManager::setMode(DisplayMode::HALF_AND_HALF, DisplayOrientation::PORTRAIT);
    
    // Update both halves at once
    DisplayManager::displayHalfAndHalf(&weather, &departures);
    
    // Update only weather half (partial update - faster)
    DisplayManager::displayHalfAndHalf(&weather, nullptr);
    
    // Update only departure half (partial update - faster)
    DisplayManager::displayHalfAndHalf(nullptr, &departures);
    
    // Alternative partial update methods
    DisplayManager::updateWeatherHalf(weather);
    DisplayManager::updateDepartureHalf(departures);
    
    // 2. Weather Only Mode - Portrait (Full screen weather)
    DisplayManager::setMode(DisplayMode::WEATHER_ONLY, DisplayOrientation::PORTRAIT);
    DisplayManager::displayWeatherOnly(weather);
    
    // 3. Departures Only Mode - Portrait (Full screen departures)
    DisplayManager::setMode(DisplayMode::DEPARTURES_ONLY, DisplayOrientation::PORTRAIT);
    DisplayManager::displayDeparturesOnly(departures);
    
    // === LANDSCAPE MODE EXAMPLES ===
    
    // 4. Half and Half Mode - Landscape (Weather left, Departures right)
    DisplayManager::setMode(DisplayMode::HALF_AND_HALF, DisplayOrientation::LANDSCAPE);
    DisplayManager::displayHalfAndHalf(&weather, &departures);
    
    // 5. Weather Only Mode - Landscape (Full screen weather)
    DisplayManager::setMode(DisplayMode::WEATHER_ONLY, DisplayOrientation::LANDSCAPE);
    DisplayManager::displayWeatherOnly(weather);
    
    // 6. Departures Only Mode - Landscape (Full screen departures)
    DisplayManager::setMode(DisplayMode::DEPARTURES_ONLY, DisplayOrientation::LANDSCAPE);
    DisplayManager::displayDeparturesOnly(departures);
    
    // === UTILITY FUNCTIONS ===
    
    // Clear specific regions - orientation-specific
    DisplayManager::clearRegion(DisplayRegion::LEFT_HALF);   // Landscape left
    DisplayManager::clearRegion(DisplayRegion::RIGHT_HALF);  // Landscape right
    DisplayManager::clearRegion(DisplayRegion::UPPER_HALF);  // Portrait upper
    DisplayManager::clearRegion(DisplayRegion::LOWER_HALF);  // Portrait lower
    
    // Clear semantic regions - adapts to current orientation
    DisplayManager::clearRegion(DisplayRegion::WEATHER_AREA);   // Weather area (left in landscape, upper in portrait)
    DisplayManager::clearRegion(DisplayRegion::DEPARTURE_AREA); // Departure area (right in landscape, lower in portrait)
    DisplayManager::clearRegion(DisplayRegion::FULL_SCREEN);
    
    // Hibernate display to save power
    DisplayManager::hibernate();
}

// Example of a typical update cycle
void updateCycle() {
    WeatherInfo weather;
    DepartureData departures;
    bool weatherSuccess = false;
    bool departureSuccess = false;
    
    // Initialize display (could be done once at startup)
    DisplayManager::init(DisplayOrientation::PORTRAIT);
    DisplayManager::setMode(DisplayMode::HALF_AND_HALF);
    
    // Fetch weather data
    if (getWeatherFromDWD(50.1109, 8.6821, weather)) { // Frankfurt coordinates
        weatherSuccess = true;
    }
    
    // Fetch departure data
    if (getDepartureFromRMV("3006907", departures)) { // Frankfurt Hauptbahnhof
        departureSuccess = true;
    }
    
    // Display based on what data we have
    if (weatherSuccess && departureSuccess) {
        // Both data available - show split screen
        DisplayManager::displayHalfAndHalf(&weather, &departures);
    } else if (weatherSuccess) {
        // Only weather available - show weather in its half
        DisplayManager::displayHalfAndHalf(&weather, nullptr);
    } else if (departureSuccess) {
        // Only departures available - show departures in its half
        DisplayManager::displayHalfAndHalf(nullptr, &departures);
    }
    
    DisplayManager::hibernate();
}

// Example of periodic updates with partial refresh
void periodicUpdate() {
    static unsigned long lastWeatherUpdate = 0;
    static unsigned long lastDepartureUpdate = 0;
    
    unsigned long now = millis();
    
    // Update weather every 10 minutes (600000 ms)
    if (now - lastWeatherUpdate > 600000) {
        WeatherInfo weather;
        if (getWeatherFromDWD(50.1109, 8.6821, weather)) {
            DisplayManager::updateWeatherHalf(weather); // Partial update - faster
            lastWeatherUpdate = now;
        }
    }
    
    // Update departures every 2 minutes (120000 ms)
    if (now - lastDepartureUpdate > 120000) {
        DepartureData departures;
        if (getDepartureFromRMV("3006907", departures)) {
            DisplayManager::updateDepartureHalf(departures); // Partial update - faster
            lastDepartureUpdate = now;
        }
    }
}

// Example demonstrating different region types
void regionExamples() {
    // === LANDSCAPE MODE REGIONS ===
    DisplayManager::init(DisplayOrientation::LANDSCAPE);
    
    // Orientation-specific regions in landscape
    DisplayManager::clearRegion(DisplayRegion::LEFT_HALF);   // Weather area (left 400px)
    DisplayManager::clearRegion(DisplayRegion::RIGHT_HALF);  // Departure area (right 400px)
    
    // === PORTRAIT MODE REGIONS ===
    DisplayManager::setMode(DisplayMode::HALF_AND_HALF, DisplayOrientation::PORTRAIT);
    
    // Orientation-specific regions in portrait
    DisplayManager::clearRegion(DisplayRegion::UPPER_HALF);  // Weather area (top 400px)
    DisplayManager::clearRegion(DisplayRegion::LOWER_HALF);  // Departure area (bottom 400px)
    
    // === SEMANTIC REGIONS (RECOMMENDED) ===
    // These automatically adapt to the current orientation:
    
    DisplayManager::clearRegion(DisplayRegion::WEATHER_AREA);   
    // In landscape: clears LEFT_HALF
    // In portrait: clears UPPER_HALF
    
    DisplayManager::clearRegion(DisplayRegion::DEPARTURE_AREA); 
    // In landscape: clears RIGHT_HALF  
    // In portrait: clears LOWER_HALF
    
    // Full screen always works the same
    DisplayManager::clearRegion(DisplayRegion::FULL_SCREEN);
}

// Configuration-driven display mode
void configBasedDisplay() {
    // You could add these to your config structure
    DisplayOrientation orientation = DisplayOrientation::PORTRAIT; // From config
    DisplayMode mode = DisplayMode::HALF_AND_HALF; // From config
    
    DisplayManager::init(orientation);
    DisplayManager::setMode(mode, orientation);
    
    // Rest of your display logic...
}
