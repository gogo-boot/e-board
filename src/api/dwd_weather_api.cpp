#include "api/dwd_weather_api.h"
#include "config/config_struct.h"
#include "config/config_manager.h"
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
    // Nominatim requires a valid User-Agent with contact info per their usage policy
    // https://operations.osmfoundation.org/policies/nominatim/
    http.addHeader("User-Agent", "MyStation-ESP32/1.0 (https://github.com/gogo-boot/mystation)");
    http.addHeader("Accept", "application/json");
    http.addHeader("Accept-Language", "en");
    // Set longer timeout - Nominatim can be slow for automated requests (15 seconds)
    http.setTimeout(5000);
    int httpCode = http.GET();
    String city = "";
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["address"].is<JsonObject>()) {
            if (doc["address"]["city"].is<const char*>()) {
                city = doc["address"]["city"].as<String>();
            } else if (doc["address"]["town"].is<const char*>()) {
                city = doc["address"]["town"].as<String>();
            } else if (doc["address"]["village"].is<const char*>()) {
                city = doc["address"]["village"].as<String>();
            } else if (doc["address"]["county"].is<const char*>()) {
                city = doc["address"]["county"].as<String>();
            }
        }
    } else {
        ESP_LOGW("DWD_CITY", "HTTP request failed, code: %d, error: %s", httpCode,
                 http.errorToString(httpCode).c_str());
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
bool getGeneralWeatherFull(float lat, float lon, WeatherInfo& weather) {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&daily=sunset,sunrise,uv_index_max,sunshine_duration,precipitation_sum,precipitation_hours,weather_code,temperature_2m_max,temperature_2m_min,apparent_temperature_min,apparent_temperature_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant"
        +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        "&current=temperature_2m,precipitation,weather_code" +
        "&timezone=auto&past_hours=0&forecast_hours=13";

    // Append weather model if configured
    RTCConfigData& config = ConfigManager::getConfig();
    if (strlen(config.weatherModel) > 0) {
        url += "&models=" + String(config.weatherModel);
    }

    ESP_LOGI(TAG, "Fetching weather from: %s\n", url.c_str());
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            // Parse current weather
            if (doc["current"].is<JsonObject>()) {
                JsonObject current = doc["current"];
                safeStringCopy(weather.time, current["time"].as<String>(), TIME_STRING_LENGTH);
                weather.temperature = current["temperature_2m"].as<float>();
                weather.precipitation = current["precipitation"].as<float>();
                weather.weatherCode = current["weather_code"].as<int>();
            }

            // Parse hourly forecast
            if (doc["hourly"].is<JsonObject>()) {
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
            if (doc["daily"].is<JsonObject>()) {
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
                // Max 7-day forecast (array size is 7)
                for (size_t i = 0; i < times.size() && count < 7; ++i) {
                    // Stop if core data is null (regional models return fewer days)
                    if (temp_max[i].isNull() || temp_min[i].isNull()) {
                        break;
                    }

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

// Fetch 19h hourly data for a specific future day (06:00-00:00 next day)
bool getWeatherForDay(float lat, float lon, int dayOffset,
                      WeatherHourlyForecast hourlyOut[], int& hourlyCount) {
    if (dayOffset < 1 || dayOffset > 6) {
        ESP_LOGE(TAG, "Invalid dayOffset %d (must be 1-6)", dayOffset);
        hourlyCount = 0;
        return false;
    }

    // Calculate target date by adding dayOffset days to today.
    // Anchor to noon to avoid DST transition edge cases (DST shifts at 02:00/03:00).
    time_t now;
    time(&now);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    tmNow.tm_hour = 12;
    tmNow.tm_min = 0;
    tmNow.tm_sec = 0;
    time_t noonToday = mktime(&tmNow);

    time_t targetDay = noonToday + (dayOffset * 86400);
    time_t nextDay = targetDay + 86400;

    struct tm tmTarget;
    struct tm tmNext;
    localtime_r(&targetDay, &tmTarget);
    localtime_r(&nextDay, &tmNext);

    // Format: YYYY-MM-DDT06:00 for start, YYYY-MM-DDT00:00 for end (next day)
    char startHour[20];
    char endHour[20];
    snprintf(startHour, sizeof(startHour), "%04d-%02d-%02dT06:00",
             tmTarget.tm_year + 1900, tmTarget.tm_mon + 1, tmTarget.tm_mday);
    snprintf(endHour, sizeof(endHour), "%04d-%02d-%02dT00:00",
             tmNext.tm_year + 1900, tmNext.tm_mon + 1, tmNext.tm_mday);

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        "&timezone=auto" +
        "&start_hour=" + String(startHour) +
        "&end_hour=" + String(endHour);

    // Append weather model if configured
    RTCConfigData& config = ConfigManager::getConfig();
    if (strlen(config.weatherModel) > 0) {
        url += "&models=" + String(config.weatherModel);
    }

    ESP_LOGI(TAG, "Fetching day %d weather: %s", dayOffset, url.c_str());

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    hourlyCount = 0;

    if (httpCode > 0) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["hourly"].is<JsonObject>()) {
            JsonObject hourly = doc["hourly"];
            JsonArray times = hourly["time"];
            JsonArray temps = hourly["temperature_2m"];
            JsonArray wcode = hourly["weather_code"];
            JsonArray rainProb = hourly["precipitation_probability"];
            JsonArray precipitation = hourly["precipitation"];
            JsonArray humidity = hourly["relative_humidity_2m"];

            int count = 0;
            for (size_t i = 0; i < times.size() && count < DAY_BROWSE_HOURLY_COUNT; ++i) {
                safeStringCopy(hourlyOut[count].time, times[i].as<String>(), TIME_STRING_LENGTH);
                hourlyOut[count].temperature = temps[i].as<float>();
                hourlyOut[count].weatherCode = wcode[i].as<int>();
                hourlyOut[count].rainChance = rainProb[i].as<int>();
                hourlyOut[count].rainfall = precipitation[i].as<float>();
                hourlyOut[count].humidity = humidity[i].as<int>();
                count++;
            }
            hourlyCount = count;

            http.end();
            ESP_LOGI(TAG, "Day %d weather: %d hourly entries fetched", dayOffset, hourlyCount);
            return true;
        } else {
            ESP_LOGE(TAG, "JSON parse error for day %d weather", dayOffset);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed for day %d weather, code: %d", dayOffset, httpCode);
    }

    http.end();
    return false;
}
