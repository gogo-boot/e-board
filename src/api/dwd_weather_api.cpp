#include "dwd_weather_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Helper: Get city/location name from lat/lon using Nominatim (OpenStreetMap)
static String getCityFromLatLon(float lat, float lon) {
    String url = "https://nominatim.openstreetmap.org/reverse?format=json&lat=" + String(lat, 6) + "&lon=" + String(lon, 6) + "&zoom=10&addressdetails=1";
    HTTPClient http;
    http.begin(url);
    http.addHeader("User-Agent", "ESP32-e-board/1.0");
    int httpCode = http.GET();
    String city = "";
    if (httpCode > 0) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc.containsKey("address")) {
            if (doc["address"].containsKey("city")) {
                city = doc["address"]["city"].as<String>();
            } else if (doc["address"].containsKey("town")) {
                city = doc["address"]["town"].as<String>();
            } else if (doc["address"].containsKey("village")) {
                city = doc["address"]["village"].as<String>();
            } else if (doc["address"].containsKey("county")) {
                city = doc["address"]["county"].as<String>();
            }
        }
    }
    http.end();
    return city;
}

bool getWeatherFromDWD(float lat, float lon, WeatherInfo &weather) {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
                 "&longitude=" + String(lon, 6) +
                 "&current_weather=true&temperature_unit=celsius&windspeed_unit=kmh";
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        weather.rawJson = payload;
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            if (doc.containsKey("current_weather")) {
                weather.temperature = String(doc["current_weather"]["temperature"].as<float>(), 1) + " °C";
                weather.condition = String(doc["current_weather"]["weathercode"].as<int>()); // You can map this code to a description
                // Get city/location name
                weather.city = getCityFromLatLon(lat, lon);
                http.end();
                return true;
            }
        }
    }
    http.end();
    return false;
}
