#include "util/util.h"
#include <vector>
#include <set>
#include <map>
#include <icons.h>

void Util::printFreeHeap(const char* msg) {
    Serial.printf("%s Free heap: %u bytes\n", msg, ESP.getFreeHeap());
}

String Util::urlEncode(const String& str) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            code0 = ((c >> 4) & 0xf) + '0';
            if (((c >> 4) & 0xf) > 9) code0 = ((c >> 4) & 0xf) - 10 + 'A';
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

String Util::urlDecode(const String& str) {
    String decoded = "";
    char temp[] = "00";
    unsigned int len = str.length();
    unsigned int i = 0;
    while (i < len) {
        char c = str.charAt(i);
        if (c == '%') {
            if (i + 2 < len) {
                temp[0] = str.charAt(i + 1);
                temp[1] = str.charAt(i + 2);
                decoded += (char)strtol(temp, nullptr, 16);
                i += 3;
            } else {
                decoded += c;
                i++;
            }
        } else if (c == '+') {
            decoded += ' ';
            i++;
        } else {
            decoded += c;
            i++;
        }
    }
    return decoded;
}

String Util::getUniqueSSID(const String& prefix) {
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-%06X", prefix.c_str(), chipId & 0xFFFFFF);
    return String(ssid);
}

// Shortens the destination name by removing common prefix tokens with the departure,
// and replacing certain words with their shorter forms using a replacements map.
//
// Example:
//   Input:  departure = "Frankfurt Hauptbahnhof", destination = "Frankfurt Hauptbahnhof Südseite"
//   Output: "Südseite"
//
//   Input:  departure = "Frankfurt", destination = "Frankfurt Bahnhof"
//   Output: "Bhf"
String Util::shortenDestination(const String departure, const String destination) {
    // Tokenize departure
    std::vector<String> depTokens;
    int start = 0, end = 0;
    while ((end = departure.indexOf(' ', start)) != -1) {
        depTokens.push_back(departure.substring(start, end));
        start = end + 1;
    }
    depTokens.push_back(departure.substring(start));

    // Tokenize destination
    std::vector<String> destTokens;
    start = 0;
    while ((end = destination.indexOf(' ', start)) != -1) {
        destTokens.push_back(destination.substring(start, end));
        start = end + 1;
    }
    destTokens.push_back(destination.substring(start));

    // Remove matching tokens from the start
    size_t i = 0;
    while (i < depTokens.size() && i < destTokens.size() && depTokens[i] == destTokens[i]) {
        ++i;
    }

    // Reconstruct string from remaining destination tokens
    String result = "";
    for (size_t j = i; j < destTokens.size(); ++j) {
        if (j > i) result += " ";
        result += destTokens[j];
    }

    // Map for replacements
    std::map<String, String> replacements = {
        {"Bahnhof", "Bhf"}, {"(Taunus)", "Ts"}, {"(Main)", "M"}, {"(Hbf)", "Hbf"}, {"(Hauptbahnhof)", "Hbf"}
    };

    // Replace map-matched replacements in result
    for (const auto& pair : replacements) {
        int idx = result.indexOf(pair.first);
        while (idx != -1) {
            result = result.substring(0, idx) + pair.second + result.substring(idx + pair.first.length());
            idx = result.indexOf(pair.first, idx + pair.second.length());
        }
    }
    return result;
}

// Map WMO weather codes to weather icons
icon_name Util::getWeatherIcon(const String& weatherCode) {
    int code = weatherCode.toInt();

    // WMO weather code mapping to available icons
    switch (code) {
    case 0: // Clear sky
        return wi_0_day_sunny;
    case 1: // Mainly clear
    case 2: // Partly cloudy
    case 3: // Overcast
        return wi_1_day_sunny_overcast;
    case 45: // Fog
    case 48: // Depositing rime fog
        return wi_45_day_fog;
    case 51: // Light drizzle
    case 53: // Moderate drizzle
    case 55: // Dense drizzle
        return wi_51_rain_mix;
    case 56: // Light freezing drizzle
    case 57: // Dense freezing drizzle
        return wi_56_rain_mix;
    case 61: // Slight rain
    case 63: // Moderate rain
    case 65: // Heavy rain
        return wi_61_rain;
    case 66: // Light freezing rain
    case 67: // Heavy freezing rain
        return wi_66_rain_mix;
    case 71: // Slight snow fall
    case 73: // Moderate snow fall
    case 75: // Heavy snow fall
        return wi_71_snow_wind;
    case 77: // Snow grains
        return wi_77_day_snow_wind;
    case 80: // Slight rain showers
    case 81: // Moderate rain showers
    case 82: // Violent rain showers
        return wi_81_showers;
    case 85: // Slight snow showers
    case 86: // Heavy snow showers
        return wi_85_snow_wind;
    case 95: // Thunderstorm
        return wi_95_thunderstorm;
    case 96: // Thunderstorm with slight hail
    case 99: // Thunderstorm with heavy hail
        return wi_99_thunderstorm;
    default:
        // Default to sunny for unknown codes
        return wi_0_day_sunny;
    }
}

// Converts wind direction in degrees to compass direction (N, NE, E, etc)
String Util::degreeToCompass(float degree) {
    static const char* directions[] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    int idx = (int)((degree + 11.25) / 22.5);
    idx = idx % 16;
    return String(directions[idx]);
}

// Converts UV index value to grade string
// UV Index Low (1-2), Moderate (3-5), High (6-7), Very High (8-10), and Extreme (11+).
String Util::uvIndexToGrade(const String& uvIndexStr) {
    float uv = uvIndexStr.toFloat();
    String grade;
    if (uv < 3) grade = "Low";
    else if (uv < 6) grade = "Moderate";
    else if (uv < 8) grade = "High";
    else if (uv < 11) grade = "Very High";
    else grade = "Extreme";
    return grade + " (" + uvIndexStr + ")";
}

// Converts sunshine duration in seconds to hh:mm format
String Util::sunshineSecondsToHHMM(const String& secondsStr) {
    long seconds = secondsStr.toFloat();
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hours, minutes);
    return String(buf);
}

// Extracts date string (YYYY-MM-DD) from a datetime string
String Util::getDateString(const String& dateStr) {
    int tIdx = dateStr.indexOf('T');
    if (tIdx > 0) return dateStr.substring(0, tIdx);
    if (dateStr.length() >= 10) return dateStr.substring(0, 10);
    return dateStr;
}

// Format wind speed and gust as "min - max m/s"
String Util::formatWindText(const String& windSpeed, const String& windGust) {
    return "Wind : " + windSpeed + " - " + windGust + " m/s";
}

// Format ISO date string (e.g., "2025-07-16T15:30") to "DD. Month" format
String Util::formatDateText(const String& isoTime) {
    int year = 0, month = 0, day = 0;
    if (isoTime.length() >= 10) {
        year = isoTime.substring(0, 4).toInt();
        month = isoTime.substring(5, 7).toInt();
        day = isoTime.substring(8, 10).toInt();
    }
    static const char* monthNames[] = {
        "", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October",
        "November", "December"
    };
    if (month > 0 && month <= 12 && day > 0) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d. %s", day, monthNames[month]);
        return String(buf);
    } else {
        return "Date: N/A";
    }
}
