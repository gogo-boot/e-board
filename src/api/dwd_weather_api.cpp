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

// Fetch full hourly data (00:00–23:00) for up to maxDays days in a SINGLE call.
// Populates cache[day][hour]. Handles limited-day models: Open-Meteo returns all
// requested days' timestamps but fills temperatures with null past a model's range;
// those hours are skipped. A day counts as valid only if hours 06:00–23:00 are all
// present (non-null), which is what the day-browse graph renders.
int getWeatherHourlyMultiDay(float lat, float lon, int maxDays,
                             DayBrowsePoint cache[][DAY_CACHE_HOURS]) {
    if (maxDays < 1) return 0;
    if (maxDays > DAY_CACHE_MAX_DAYS) maxDays = DAY_CACHE_MAX_DAYS;

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
        "&longitude=" + String(lon, 6) +
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,relative_humidity_2m" +
        "&timezone=auto" +
        "&forecast_days=" + String(maxDays);

    RTCConfigData& config = ConfigManager::getConfig();
    if (strlen(config.weatherModel) > 0) {
        url += "&models=" + String(config.weatherModel);
    }

    ESP_LOGI(TAG, "Fetching multi-day hourly (%d days): %s", maxDays, url.c_str());

    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
    int httpCode = http.GET();

    if (httpCode <= 0) {
        ESP_LOGE(TAG, "Multi-day hourly HTTP failed, code: %d", httpCode);
        http.end();
        return 0;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error || !doc["hourly"].is<JsonObject>()) {
        ESP_LOGE(TAG, "Multi-day hourly JSON parse error");
        return 0;
    }

    JsonObject hourly = doc["hourly"];
    JsonArray times     = hourly["time"];
    JsonArray temps     = hourly["temperature_2m"];
    JsonArray wcode     = hourly["weather_code"];
    JsonArray rainProb  = hourly["precipitation_probability"];
    JsonArray precip    = hourly["precipitation"];
    JsonArray humidity  = hourly["relative_humidity_2m"];

    // Track which (day, hour) slots got a valid (non-null) value.
    bool filled[DAY_CACHE_MAX_DAYS][DAY_CACHE_HOURS] = {{false}};

    // Timestamps are contiguous and ordered starting at day 0 hour 0 (00:00).
    // Index → day/hour: day = i / 24, hour = i % 24.
    for (size_t i = 0; i < times.size(); ++i) {
        int day  = (int)(i / DAY_CACHE_HOURS);
        int hour = (int)(i % DAY_CACHE_HOURS);
        if (day >= maxDays) break;

        // Skip hours the model does not provide (null temperature).
        if (temps[i].isNull()) continue;

        DayBrowsePoint& p = cache[day][hour];
        p.temperature = temps[i].as<float>();
        p.rainfall    = precip[i].isNull() ? 0.0f : precip[i].as<float>();

        int rc = rainProb[i].isNull() ? 0 : rainProb[i].as<int>();
        if (rc < 0) rc = 0; if (rc > 100) rc = 100;
        p.rainChance = (int16_t)rc;

        p.weatherCode = wcode[i].isNull() ? 0 : (uint8_t)wcode[i].as<int>();

        int hum = humidity[i].isNull() ? 0 : humidity[i].as<int>();
        if (hum < 0) hum = 0; if (hum > 100) hum = 100;
        p.humidity = (uint8_t)hum;

        filled[day][hour] = true;
    }

    // A day is valid only if the entire display window (06:00–23:00) is present.
    int validDays = 0;
    for (int day = 0; day < maxDays; ++day) {
        bool windowComplete = true;
        for (int h = DAY_BROWSE_START_HOUR; h < DAY_CACHE_HOURS; ++h) {
            if (!filled[day][h]) { windowComplete = false; break; }
        }
        if (windowComplete) {
            validDays = day + 1;   // contiguous from day 0
        } else {
            break;                 // stop at first incomplete day
        }
    }

    ESP_LOGI(TAG, "Multi-day hourly: %d valid days cached (of %d requested)", validDays, maxDays);
    return validDays;
}
