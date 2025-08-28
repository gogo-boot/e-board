#include "api/dwd_weather_api.h"
#include "config/config_struct.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_log.h>

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

bool getGeneralWeatherHalf(float lat, float lon, WeatherInfo& weather) {
    //https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weather_code,sunrise,sunset,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,sunshine_duration&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m&current=temperature_2m,precipitation,weather_code&timezone=auto&past_hours=1&forecast_hours=12
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&daily=weather_code,sunrise,sunset,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,sunshine_duration,wind_speed_10m_max"
        +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        // Add relative_humidity_2m
        "&current=temperature_2m,precipitation,weather_code" +
        "&timezone=auto&past_hours=0&forecast_hours=13";
    Serial.printf("Fetching weather from: %s\n", url.c_str());
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
                weather.time = current["time"].as<String>();
                weather.temperature = String(current["temperature_2m"].as<float>(), 1);
                weather.precipitation = String(current["precipitation"].as<float>(), 1);
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
                JsonArray humidity = hourly["relative_humidity_2m"]; // Add humidity array

                int count = 0;
                for (size_t i = 0; i < times.size() && count < 13; ++i) {
                    weather.hourlyForecast[count].time = times[i].as<String>();
                    weather.hourlyForecast[count].temperature = String(temps[i].as<float>(), 1);
                    weather.hourlyForecast[count].weatherCode = String(wcode[i].as<int>());
                    weather.hourlyForecast[count].rainChance = String(rainProb[i].as<int>());
                    weather.hourlyForecast[count].rainfall = String(precipitation[i].as<float>(), 2);
                    weather.hourlyForecast[count].humidity = String(humidity[i].as<int>()); // Parse humidity

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
                JsonArray precipitation = daily["precipitation_sum"];

                JsonArray wcode = daily["weather_code"];
                JsonArray temp_max = daily["temperature_2m_max"];
                JsonArray temp_min = daily["temperature_2m_min"];
                JsonArray wind_speed_10m_max = daily["wind_speed_10m_max"];

                // Extract time from ISO format (e.g., "2025-07-13T04:59") to "04:59"
                auto extractTimeFromISO = [](const String& isoDateTime) -> String {
                    int tIndex = isoDateTime.indexOf('T');
                    if (tIndex > 0) {
                        return isoDateTime.substring(tIndex + 1, tIndex + 6);
                    }
                    return isoDateTime;
                };
                int count = 0;
                // Max 14-day forecast
                for (size_t i = 0; i < times.size() && count < 14; ++i) {
                    weather.dailyForecast[count].time = times[i].as<String>();
                    weather.dailyForecast[count].sunset = extractTimeFromISO(String(sunset[i].as<String>()));
                    weather.dailyForecast[count].sunrise = extractTimeFromISO(String(sunrise[i].as<String>()));

                    weather.dailyForecast[count].uvIndex = String(uv_index[i].as<float>(), 1);
                    weather.dailyForecast[count].sunshineDuration = String(sunshine[i].as<float>(), 2);
                    weather.dailyForecast[count].precipitationSum = String(precipitation[i].as<float>(), 1);

                    weather.dailyForecast[count].weatherCode = String(wcode[i].as<int>());
                    weather.dailyForecast[count].tempMax = String(temp_max[i].as<float>(), 1);
                    weather.dailyForecast[count].tempMin = String(temp_min[i].as<float>(), 1);
                    weather.dailyForecast[count].windSpeedMax = String(wind_speed_10m_max[i].as<float>(), 1);

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

bool getGeneralWeatherFull(float lat, float lon, WeatherInfo& weather) {
    // https://api.open-meteo.com/v1/forecast?latitude=50.125599&longitude=8.604095
    // &daily=sunset,sunrise,uv_index_max,sunshine_duration,precipitation_sum,precipitation_hours,weather_code,temperature_2m_max,temperature_2m_min,apparent_temperature_min,apparent_temperature_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant
    // &hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m
    // &current=temperature_2m,precipitation,weather_code
    // &timezone=auto&past_hours=0&forecast_hours=13
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&daily=sunset,sunrise,uv_index_max,sunshine_duration,precipitation_sum,precipitation_hours,weather_code,temperature_2m_max,temperature_2m_min,apparent_temperature_min,apparent_temperature_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant"
        +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        "&current=temperature_2m,precipitation,weather_code" +
        "&timezone=auto&past_hours=0&forecast_hours=13";
    Serial.printf("Fetching weather from: %s\n", url.c_str());
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
                weather.time = current["time"].as<String>();
                weather.temperature = String(current["temperature_2m"].as<float>(), 1);
                weather.precipitation = String(current["precipitation"].as<float>(), 1);
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
                    weather.hourlyForecast[count].time = times[i].as<String>();
                    weather.hourlyForecast[count].temperature = String(temps[i].as<float>(), 1);
                    weather.hourlyForecast[count].weatherCode = String(wcode[i].as<int>());
                    weather.hourlyForecast[count].rainChance = String(rainProb[i].as<int>());
                    weather.hourlyForecast[count].rainfall = String(precipitation[i].as<float>(), 2);
                    weather.hourlyForecast[count].humidity = String(humidity[i].as<int>()); // Parse humidity

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

                // Extract time from ISO format (e.g., "2025-07-13T04:59") to "04:59"
                auto extractTimeFromISO = [](const String& isoDateTime) -> String {
                    int tIndex = isoDateTime.indexOf('T');
                    if (tIndex > 0) {
                        return isoDateTime.substring(tIndex + 1, tIndex + 6);
                    }
                    return isoDateTime;
                };
                int count = 0;
                // Max 14-day forecast
                for (size_t i = 0; i < times.size() && count < 14; ++i) {
                    weather.dailyForecast[count].time = times[i].as<String>();
                    weather.dailyForecast[count].sunset = extractTimeFromISO(String(sunset[i].as<String>()));
                    weather.dailyForecast[count].sunrise = extractTimeFromISO(String(sunrise[i].as<String>()));
                    weather.dailyForecast[count].uvIndex = String(uv_index[i].as<float>(), 1);

                    weather.dailyForecast[count].sunshineDuration = String(sunshine[i].as<float>(), 2);
                    weather.dailyForecast[count].precipitationSum = String(precipitation_sum[i].as<int>());
                    weather.dailyForecast[count].precipitationHours = String(precipitation_hours[i].as<int>());
                    weather.dailyForecast[count].weatherCode = String(wcode[i].as<int>());

                    weather.dailyForecast[count].tempMax = String(temp_max[i].as<float>(), 1);
                    weather.dailyForecast[count].tempMin = String(temp_min[i].as<float>(), 1);
                    weather.dailyForecast[count].apparentTempMin = String(apparent_temp_min[i].as<float>(), 1);
                    weather.dailyForecast[count].apparentTempMax = String(apparent_temp_max[i].as<float>(), 1);
                    weather.dailyForecast[count].windSpeedMax = String(wind_speed_10m_max[i].as<float>(), 1);

                    weather.dailyForecast[count].windGustsMax = String(wind_gusts_10m_max[i].as<float>(), 1);
                    weather.dailyForecast[count].windDirection = String(windDirection[i].as<int>());

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
