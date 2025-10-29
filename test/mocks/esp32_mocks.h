#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstring>

// Mock ESP32 logging
#define ESP_LOGI(tag, format, ...) printf("[INFO][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[WARN][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("[ERROR][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) printf("[DEBUG][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) printf("[VERBOSE][%s] " format "\n", tag, ##__VA_ARGS__)
#define RTC_DATA_ATTR

// Mock Arduino String class
class String : public std::string {
public:
    String() = default;
    String(const char* str) : std::string(str ? str : "") {}
    String(const std::string& str) : std::string(str) {}
    String(int val) : std::string(std::to_string(val)) {}
    String(unsigned int val) : std::string(std::to_string(val)) {}
    String(long val) : std::string(std::to_string(val)) {}
    String(unsigned long val) : std::string(std::to_string(val)) {}
    String(float val) : std::string(std::to_string(val)) {}
    String(double val) : std::string(std::to_string(val)) {}

    int indexOf(char c) const {
        size_t pos = find(c);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    String substring(int start) const {
        if (start < 0 || start >= static_cast<int>(length())) return String();
        return String(substr(start));
    }

    String substring(int start, int end) const {
        if (start < 0 || start >= static_cast<int>(length())) return String();
        if (end < start || end > static_cast<int>(length())) end = length();
        return String(substr(start, end - start));
    }

    int toInt() const {
        try { return std::stoi(*this); } catch (...) { return 0; }
    }

    float toFloat() const {
        try { return std::stof(*this); } catch (...) { return 0.0f; }
    }

    int length() const { return static_cast<int>(size()); }

    const char* c_str() const { return std::string::c_str(); }
};

// Ensure min/max are available
using std::min;
using std::max;
