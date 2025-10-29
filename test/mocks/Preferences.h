#pragma once

#include <string>
#include <map>
#include <cstring>

// Mock Preferences class for native testing
class Preferences {
public:
    Preferences() = default;

    bool begin(const char* name, bool readOnly = false) {
        namespace_name = name;
        return true;
    }

    void end() {
        // No-op for mock
    }

    void clear() {
        storage.clear();
    }

    bool remove(const char* key) {
        return storage.erase(key) > 0;
    }

    // Getters
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        auto it = storage.find(key);
        if (it != storage.end() && it->second.size() == sizeof(uint8_t)) {
            return *reinterpret_cast<const uint8_t*>(it->second.data());
        }
        return defaultValue;
    }

    int getInt(const char* key, int defaultValue = 0) {
        auto it = storage.find(key);
        if (it != storage.end() && it->second.size() == sizeof(int)) {
            return *reinterpret_cast<const int*>(it->second.data());
        }
        return defaultValue;
    }

    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        auto it = storage.find(key);
        if (it != storage.end() && it->second.size() == sizeof(uint32_t)) {
            return *reinterpret_cast<const uint32_t*>(it->second.data());
        }
        return defaultValue;
    }

    float getFloat(const char* key, float defaultValue = 0.0f) {
        auto it = storage.find(key);
        if (it != storage.end() && it->second.size() == sizeof(float)) {
            return *reinterpret_cast<const float*>(it->second.data());
        }
        return defaultValue;
    }

    bool getBool(const char* key, bool defaultValue = false) {
        auto it = storage.find(key);
        if (it != storage.end() && it->second.size() == sizeof(bool)) {
            return *reinterpret_cast<const bool*>(it->second.data());
        }
        return defaultValue;
    }

    size_t getString(const char* key, char* value, size_t maxLen) {
        auto it = storage.find(key);
        if (it != storage.end()) {
            size_t len = std::min(it->second.size(), maxLen - 1);
            std::memcpy(value, it->second.data(), len);
            value[len] = '\0';
            return len;
        }
        value[0] = '\0';
        return 0;
    }

    std::string getString(const char* key, const std::string& defaultValue = "") {
        auto it = storage.find(key);
        if (it != storage.end()) {
            return std::string(it->second.begin(), it->second.end());
        }
        return defaultValue;
    }

    // Setters
    size_t putUChar(const char* key, uint8_t value) {
        storage[key] = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&value),
                                            reinterpret_cast<uint8_t*>(&value) + sizeof(uint8_t));
        return sizeof(uint8_t);
    }

    size_t putInt(const char* key, int value) {
        storage[key] = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&value),
                                            reinterpret_cast<uint8_t*>(&value) + sizeof(int));
        return sizeof(int);
    }

    size_t putUInt(const char* key, uint32_t value) {
        storage[key] = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&value),
                                            reinterpret_cast<uint8_t*>(&value) + sizeof(uint32_t));
        return sizeof(uint32_t);
    }

    size_t putFloat(const char* key, float value) {
        storage[key] = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&value),
                                            reinterpret_cast<uint8_t*>(&value) + sizeof(float));
        return sizeof(float);
    }

    size_t putBool(const char* key, bool value) {
        storage[key] = std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&value),
                                            reinterpret_cast<uint8_t*>(&value) + sizeof(bool));
        return sizeof(bool);
    }

    size_t putString(const char* key, const char* value) {
        storage[key] = std::vector<uint8_t>(value, value + strlen(value));
        return strlen(value);
    }

    size_t putString(const char* key, const std::string& value) {
        storage[key] = std::vector<uint8_t>(value.begin(), value.end());
        return value.size();
    }

private:
    std::string namespace_name;
    std::map<std::string, std::vector<uint8_t>> storage;
};

