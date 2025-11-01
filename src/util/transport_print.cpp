#include "util/transport_print.h"
#include <Arduino.h>
#include <esp_log.h>

// Include e-paper display libraries
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

// External display instance from main.cpp
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;

static const char* TAG = "TRANSPORT";

void printTransportInfo(const DepartureData& depart) {
    ESP_LOGI(TAG, "--- TransportInfo ---");
    ESP_LOGI(TAG, "Stop: %s (%s)", depart.stopName.c_str(), depart.stopId.c_str());
    ESP_LOGI(TAG, "Departure count: %d", depart.departureCount);

    for (int i = 0; i < depart.departureCount && i < 10; ++i) {
        const auto& dep = depart.departures[i];
        ESP_LOGI(
            TAG,
            "Departure %d | Line: %s | Direction: %s | Direction Flag: %s | Time: %s | RT Time: %s | Cancelled: %s | Track: %s | Category: %s",
            i + 1,
            dep.line.c_str(),
            dep.direction.c_str(),
            dep.directionFlag.c_str(),
            dep.time.c_str(),
            dep.rtTime.c_str(),
            dep.cancelled ? "true" : "false",
            dep.track.c_str(),
            dep.category.c_str());
    }
    ESP_LOGI(TAG, "--- End TransportInfo ---");
}
