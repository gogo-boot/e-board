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
//  https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=temperature_2m_max,temperature_2m_min,weather_code,sunrise,sunset&hourly=temperature_2m,precipitation_probability,relative_humidity_2m,wind_speed_10m,weather_code,rain,snowfall&timezone=auto&forecast_days=1&past_hours=1&forecast_hours=12

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
                 "&longitude=" + String(lon, 6) +
                 "&daily=temperature_2m_max,temperature_2m_min,weather_code,sunrise,sunset&hourly=temperature_2m,precipitation_probability,relative_humidity_2m,wind_speed_10m,weather_code,rain,snowfall&timezone=auto&forecast_days=1&past_hours=1&forecast_hours=12";
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
            if (doc.containsKey("current_weather")) {
                weather.temperature = String(doc["current_weather"]["temperature"].as<float>(), 1) + " °C";
                int wcode = doc["current_weather"]["weathercode"].as<int>();
                weather.condition = weatherCodeToString(wcode);
                if (weather.city.isEmpty()) {
                    getCityFromLatLon(lat, lon);
                }
            }
            if (doc.containsKey("hourly")) {
                JsonObject hourly = doc["hourly"];
                JsonArray times = hourly["time"];
                JsonArray temps = hourly["temperature_2m"];
                JsonArray rain_prob = hourly["precipitation_probability"];
                JsonArray humidity = hourly["relative_humidity_2m"];
                JsonArray wind = hourly["wind_speed_10m"];
                JsonArray rainfall = hourly["rain"];
                JsonArray snowfall = hourly["snowfall"];
                JsonArray wcode = hourly["weather_code"];
                int count = 0;
                for (size_t i = 0; i < times.size() && count < 12; ++i) {
                    weather.forecast[count].time = times[i].as<String>();
                    weather.forecast[count].temperature = String(temps[i].as<float>(), 1) + " °C";
                    weather.forecast[count].rainChance = String(rain_prob[i].as<float>(), 0) + "%";
                    weather.forecast[count].humidity = String(humidity[i].as<float>(), 0) + "%";
                    weather.forecast[count].windSpeed = String(wind[i].as<float>(), 1) + " km/h";
                    weather.forecast[count].rainfall = String(rainfall[i].as<float>(), 1) + " mm";
                    weather.forecast[count].snowfall = String(snowfall[i].as<float>(), 1) + " mm";
                    int code = wcode[i].as<int>();
                    weather.forecast[count].weatherCode = String(code);
                    weather.forecast[count].weatherDesc = weatherCodeToString(code);
                    count++;
                }
                weather.forecastCount = count;
            }
            if (doc.containsKey("daily")) {
                JsonObject daily = doc["daily"];
                JsonArray tem_max = daily["temperature_2m_max"];
                JsonArray tem_min = daily["temperature_2m_min"];
                JsonArray sunrise = daily["sunrise"];
                JsonArray sunset = daily["sunset"];
                // Use first value (today)
                if (tem_max.size() > 0) weather.tempMax = String(tem_max[0].as<float>(), 1) + " °C";
                if (tem_min.size() > 0) weather.tempMin = String(tem_min[0].as<float>(), 1) + " °C";
                if (sunrise.size() > 0) weather.sunrise = sunrise[0].as<String>();
                if (sunset.size() > 0) weather.sunset = sunset[0].as<String>();
            }
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}
