#pragma once
#include <Arduino.h>
#include <vector>

/**
 * @brief Encapsulated configuration page data management
 *
 * Replaces global g_webConfigPageData with a singleton pattern for better
 * encapsulation, testability, and thread safety.
 */
class ConfigPageData {
public:
    static ConfigPageData& getInstance();

    // Location management
    void setLocation(float lat, float lon, const String& city);
    float getLatitude() const { return latitude; }
    float getLongitude() const { return longitude; }
    const String& getCityName() const { return cityName; }

    // Stop management
    void clearStops();
    void addStop(const String& id, const String& name, const String& distance);
    size_t getStopCount() const { return stopNames.size(); }
    const String& getStopId(size_t index) const;
    const String& getStopName(size_t index) const;
    const String& getStopDistance(size_t index) const;
    // IP address management
    void setIPAddress(const String& ip) { ipAddress = ip; }
    const String& getIPAddress() const { return ipAddress; }

private:
    ConfigPageData() = default;

    // Location data
    float latitude = 0.0;
    float longitude = 0.0;
    String cityName;
    String ipAddress;

    // Stop data
    std::vector<String> stopIds;
    std::vector<String> stopNames;
    std::vector<String> stopDistances;
};

