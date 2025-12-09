#include "api/dwd_weather_api.h"
#include "config/config_struct.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_log.h>

static const char* TAG = "WEATHER_API";

// Get city/location name from lat/lon using Nominatim (OpenStreetMap)
String getCityFromLatLon(float lat, float lon) {
    String url = "https://nominatim.openstreetmap.org/reverse?format=json&lat=" + String(lat, 6) + "&lon=" +
        String(lon, 6) + "&zoom=10&addressdetails=1";
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
    case 1:
    case 2:
    case 3: return "Mainly clear, partly cloudy, overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 56:
    case 57: return "Freezing Drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 66:
    case 67: return "Freezing Rain";
    case 71:
    case 73:
    case 75: return "Snow fall";
    case 77: return "Snow grains";
    case 80:
    case 81:
    case 82: return "Rain showers";
    case 85:
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Thunderstorm with hail";
    default: return "Unknown";
    }
}

bool getGeneralWeatherFull(float lat, float lon, WeatherInfo& weather) {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&daily=sunset,sunrise,uv_index_max,sunshine_duration,precipitation_sum,precipitation_hours,weather_code,temperature_2m_max,temperature_2m_min,apparent_temperature_min,apparent_temperature_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant"
        +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        "&current=temperature_2m,precipitation,weather_code" +
        "&timezone=auto&past_hours=0&forecast_hours=13";
    ESP_LOGI(TAG, "Fetching weather from: %s\n", url.c_str());
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            // Parse current weather
            if (doc.containsKey("current")) {
                JsonObject current = doc["current"];
                safeStringCopy(weather.time, current["time"].as<String>(), TIME_STRING_LENGTH);
                weather.temperature = current["temperature_2m"].as<float>();
                weather.precipitation = current["precipitation"].as<float>();
                weather.weatherCode = current["weather_code"].as<int>();
            }

            // Parse hourly forecast
            if (doc.containsKey("hourly")) {
                JsonObject hourly = doc["hourly"];

                JsonArray times = hourly["time"];
                JsonArray temps = hourly["temperature_2m"];
                JsonArray wcode = hourly["weather_code"];
                JsonArray rainProb = hourly["precipitation_probability"];
                JsonArray precipitation = hourly["precipitation"];
                JsonArray humidity = hourly["relative_humidity_2m"];

                int count = 0;
                for (size_t i = 0; i < times.size() && count < 13; ++i) {
                    safeStringCopy(weather.hourlyForecast[count].time, times[i].as<String>(), TIME_STRING_LENGTH);
                    weather.hourlyForecast[count].temperature = temps[i].as<float>();
                    weather.hourlyForecast[count].weatherCode = wcode[i].as<int>();
                    weather.hourlyForecast[count].rainChance = rainProb[i].as<int>();
                    weather.hourlyForecast[count].rainfall = precipitation[i].as<float>();
                    weather.hourlyForecast[count].humidity = humidity[i].as<int>();

                    count++;
                }
                weather.hourlyForecastCount = count;
            }

            // Parse daily data
            if (doc.containsKey("daily")) {
                JsonObject daily = doc["daily"];

                JsonArray times = daily["time"];
                JsonArray sunset = daily["sunset"];
                JsonArray sunrise = daily["sunrise"];

                JsonArray uv_index = daily["uv_index_max"];
                JsonArray sunshine = daily["sunshine_duration"];
                JsonArray precipitation_sum = daily["precipitation_sum"];
                JsonArray precipitation_hours = daily["precipitation_hours"];

                JsonArray wcode = daily["weather_code"];
                JsonArray temp_max = daily["temperature_2m_max"];
                JsonArray temp_min = daily["temperature_2m_min"];

                JsonArray apparent_temp_min = daily["apparent_temperature_min"];
                JsonArray apparent_temp_max = daily["apparent_temperature_max"];
                JsonArray wind_speed_10m_max = daily["wind_speed_10m_max"];
                JsonArray wind_gusts_10m_max = daily["wind_gusts_10m_max"];
                JsonArray windDirection = daily["wind_direction_10m_dominant"];


                int count = 0;
                // Max 14-day forecast
                for (size_t i = 0; i < times.size() && count < 14; ++i) {
                    safeStringCopy(weather.dailyForecast[count].time, times[i].as<String>(), TIME_STRING_LENGTH);

                    // Extract sunrise/sunset times
                    extractTimeFromISO(weather.dailyForecast[count].sunrise,
                                       sunrise[i].as<String>(), TIME_SHORT_LENGTH);
                    extractTimeFromISO(weather.dailyForecast[count].sunset,
                                       sunset[i].as<String>(),
                                       TIME_SHORT_LENGTH);

                    weather.dailyForecast[count].uvIndex = uv_index[i].as<float>();

                    weather.dailyForecast[count].sunshineDuration = sunshine[i].as<float>();
                    weather.dailyForecast[count].precipitationSum = precipitation_sum[i].as<float>();
                    weather.dailyForecast[count].precipitationHours = precipitation_hours[i].as<int>();
                    weather.dailyForecast[count].weatherCode = wcode[i].as<int>();

                    weather.dailyForecast[count].tempMax = temp_max[i].as<float>();
                    weather.dailyForecast[count].tempMin = temp_min[i].as<float>();
                    weather.dailyForecast[count].apparentTempMin = apparent_temp_min[i].as<float>();
                    weather.dailyForecast[count].apparentTempMax = apparent_temp_max[i].as<float>();
                    weather.dailyForecast[count].windSpeedMax = wind_speed_10m_max[i].as<float>();

                    weather.dailyForecast[count].windGustsMax = wind_gusts_10m_max[i].as<float>();
                    weather.dailyForecast[count].windDirection = windDirection[i].as<int>();

                    count++;
                }
                weather.dailyForecastCount = count;
            }

            http.end();
            return true;
        }
    }
    http.end();
    return false;
}

// Safe string copy with size checking
void safeStringCopy(char* dest, const String& src, size_t destSize) {
    size_t len = src.length();
    if (len >= destSize) {
        len = destSize - 1; // Leave room for null terminator
    }
    strncpy(dest, src.c_str(), len);
    dest[len] = '\0'; // Ensure null termination
}

// Extract time from ISO format ("2025-08-25T22:00" -> "22:00")
void extractTimeFromISO(char* dest, const String& isoDateTime, size_t destSize) {
    int tIndex = isoDateTime.indexOf('T');
    if (tIndex > 0 && (size_t)(isoDateTime.length() - tIndex - 1) < destSize) {
        String timeOnly = isoDateTime.substring(tIndex + 1, tIndex + 6);
        safeStringCopy(dest, timeOnly, destSize);
    } else {
        strncpy(dest, "00:00", destSize - 1);
        dest[destSize - 1] = '\0';
    }
}
