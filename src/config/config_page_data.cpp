#include "config/config_page_data.h"
#include <esp_log.h>

static const char* TAG = "CONFIG_PAGE_DATA";

ConfigPageData& ConfigPageData::getInstance() {
    static ConfigPageData instance;
    return instance;
}

void ConfigPageData::setLocation(float lat, float lon, const String& city) {
    latitude = lat;
    longitude = lon;
    cityName = city;
    ESP_LOGI(TAG, "Location set: %s (%.6f, %.6f)", city.c_str(), lat, lon);
}

void ConfigPageData::clearStops() {
    stopIds.clear();
    stopNames.clear();
    stopDistances.clear();
    ESP_LOGD(TAG, "Stops cleared");
}

void ConfigPageData::addStop(const String& id, const String& name, const String& distance) {
    stopIds.push_back(id);
    stopNames.push_back(name);
    stopDistances.push_back(distance);
    ESP_LOGD(TAG, "Stop added: %s (%s) - %sm", name.c_str(), id.c_str(), distance.c_str());
}

const String& ConfigPageData::getStopId(size_t index) const {
    static String empty;
    return (index < stopIds.size()) ? stopIds[index] : empty;
}

const String& ConfigPageData::getStopName(size_t index) const {
    static String empty;
    return (index < stopNames.size()) ? stopNames[index] : empty;
}

const String& ConfigPageData::getStopDistance(size_t index) const {
    static String empty;
    return (index < stopDistances.size()) ? stopDistances[index] : empty;
}

