#include "api/dwd_weather_api.h"
#include "config/config_struct.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_log.h>

// Get city/location name from lat/lon using Nominatim (OpenStreetMap)
String getCityFromLatLon(float lat, float lon) {
    String url = "https://nominatim.openstreetmap.org/reverse?format=json&lat=" + String(lat, 6) + "&lon=" + String(lon, 6) + "&zoom=10&addressdetails=1";
    HTTPClient http;
    http.begin(url);
    http.addHeader("User-Agent", "ESP32-e-board/1.0");
    int httpCode = http.GET();
    String city = "";
    if (httpCode > 0) {
        String payload = http.getString();
        ESP_LOGD("DWD_CITY", "Nominatim payload: %s", payload.c_str());
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
    // return city if found, otherwise empty string
    if (city.isEmpty()) {
        ESP_LOGW("DWD_CITY", "No city found for lat: %.6f, lon: %.6f", lat, lon);
        return "";
    }
    ESP_LOGI("DWD_CITY", "Found city: %s for lat: %.6f, lon: %.6f", city.c_str(), lat, lon);
    return city;
}

// Map Open-Meteo weather codes to human-readable strings
static String weatherCodeToString(int code) {
    switch (code) {
        case 0: return "Clear sky";
        case 1: case 2: case 3: return "Mainly clear, partly cloudy, overcast";
        case 45: case 48: return "Fog";
        case 51: case 53: case 55: return "Drizzle";
        case 56: case 57: return "Freezing Drizzle";
        case 61: case 63: case 65: return "Rain";
        case 66: case 67: return "Freezing Rain";
        case 71: case 73: case 75: return "Snow fall";
        case 77: return "Snow grains";
        case 80: case 81: case 82: return "Rain showers";
        case 85: case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm with hail";
        default: return "Unknown";
    }
}

bool getWeatherFromDWD(float lat, float lon, WeatherInfo &weather) {

//https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weather_code,sunrise,sunset,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,sunshine_duration&hourly=temperature_2m,weather_code,precipitation_probability,precipitation&current=temperature_2m,precipitation,weather_code&timezone=auto&past_hours=1&forecast_hours=12
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
                 "&longitude=" + String(lon, 6) +
                 "&daily=weather_code,sunrise,sunset,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,sunshine_duration" +
                 "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation" + 
                 "&current=temperature_2m,precipitation,weather_code" + 
                 "&timezone=auto&past_hours=1&forecast_hours=12";
    Serial.printf("Fetching weather from: %s\n", url.c_str());
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        weather.rawJson = payload;
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            // Parse current weather
            if (doc.containsKey("current")) {
                JsonObject current = doc["current"];
                weather.temperature = String(current["temperature_2m"].as<float>(), 1);
                int wcode = current["weather_code"].as<int>();
                weather.condition = weatherCodeToString(wcode);
            }
            
            // Parse hourly forecast
            if (doc.containsKey("hourly")) {
                JsonObject hourly = doc["hourly"];
                JsonArray times = hourly["time"];
                JsonArray temps = hourly["temperature_2m"];
                JsonArray rain_prob = hourly["precipitation_probability"];
                JsonArray wcode = hourly["weather_code"];
                JsonArray precipitation = hourly["precipitation"];
                
                int count = 0;
                for (size_t i = 0; i < times.size() && count < 12; ++i) {
                    weather.forecast[count].time = times[i].as<String>();
                    weather.forecast[count].temperature = String(temps[i].as<float>(), 1);
                    weather.forecast[count].rainChance = String(rain_prob[i].as<int>());
                    weather.forecast[count].rainfall = String(precipitation[i].as<float>(), 2);
                    
                    int code = wcode[i].as<int>();
                    weather.forecast[count].weatherCode = String(code);
                    weather.forecast[count].weatherDesc = weatherCodeToString(code);
                    
                    // Set default values for fields not in this API
                    weather.forecast[count].humidity = "0";
                    weather.forecast[count].windSpeed = "0";
                    weather.forecast[count].snowfall = "0";
                    
                    count++;
                }
                weather.forecastCount = count;
            }
            
            // Parse daily data
            if (doc.containsKey("daily")) {
                JsonObject daily = doc["daily"];
                JsonArray temp_max = daily["temperature_2m_max"];
                JsonArray temp_min = daily["temperature_2m_min"];
                JsonArray sunrise_arr = daily["sunrise"];
                JsonArray sunset_arr = daily["sunset"];
                JsonArray uv_index = daily["uv_index_max"];
                
                // Use first value (today)
                if (temp_max.size() > 0) weather.tempMax = String(temp_max[0].as<float>(), 1);
                if (temp_min.size() > 0) weather.tempMin = String(temp_min[0].as<float>(), 1);
                if (sunrise_arr.size() > 0) {
                    String sunrise_iso = sunrise_arr[0].as<String>();
                    // Extract time from ISO format (2025-07-13T04:59 -> 04:59)
                    int tIndex = sunrise_iso.indexOf('T');
                    if (tIndex > 0) {
                        weather.sunrise = sunrise_iso.substring(tIndex + 1, tIndex + 6);
                    } else {
                        weather.sunrise = sunrise_iso;
                    }
                }
                if (sunset_arr.size() > 0) {
                    String sunset_iso = sunset_arr[0].as<String>();
                    // Extract time from ISO format (2025-07-13T21:24 -> 21:24)
                    int tIndex = sunset_iso.indexOf('T');
                    if (tIndex > 0) {
                        weather.sunset = sunset_iso.substring(tIndex + 1, tIndex + 6);
                    } else {
                        weather.sunset = sunset_iso;
                    }
                }
                if (uv_index.size() > 0) weather.uvIndex = String(uv_index[0].as<float>(), 1);
            }
            
            // Set city if not already set
            if (weather.city.isEmpty()) {
                weather.city = getCityFromLatLon(lat, lon);
            }
            
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}
