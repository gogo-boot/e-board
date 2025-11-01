#include "util/battery_manager.h"
#include <esp_log.h>

#ifdef BOARD_ESP32_S3
#include <esp_adc_cal.h>
#endif

static const char* TAG = "BATTERY_MGR";

// Battery voltage thresholds (for LiPo batteries)
static const float BATTERY_VOLTAGE_MAX = 4.2f; // Fully charged
static const float BATTERY_VOLTAGE_MIN = 3.0f; // Empty (safe cutoff)
static const float BATTERY_VOLTAGE_NOMINAL = 3.7f; // Nominal voltage

#ifdef BOARD_ESP32_S3
// TRMNL 7.5" (OG) DIY Kit specific pins
// Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
static const int BATTERY_ADC_PIN = 1; // GPIO1 (A0) - BAT_ADC
static const int ADC_EN_PIN = 6; // GPIO6 (A5) - ADC_EN (power control)
static const float VOLTAGE_DIVIDER_RATIO = 2.0f; // 2:1 voltage divider
static const int ADC_MAX_VALUE = 4095; // 12-bit ADC
static const float ADC_REFERENCE_VOLTAGE = 3.6f; // Actual reference voltage for ESP32-S3
static const float CALIBRATION_FACTOR = 0.968f; // Calibration factor from OG DIY Kit
static bool batteryInitialized = false;
#endif

void BatteryManager::init() {
#ifdef BOARD_ESP32_S3
    ESP_LOGI(TAG, "Initializing battery manager for TRMNL OG DIY Kit (ESP32-S3)");

    // Configure ADC enable pin (power control for battery ADC circuit)
    pinMode(ADC_EN_PIN, OUTPUT);
    digitalWrite(ADC_EN_PIN, LOW); // Start with ADC disabled to save power

    // Configure ADC pin for battery voltage reading
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogReadResolution(12); // 12-bit resolution (0-4095)
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db); // Full range: 0-3.6V
    batteryInitialized = true;

    // Log initial battery status
    float voltage = getBatteryVoltage();
    int percentage = getBatteryPercentage();
    ESP_LOGI(TAG, "Battery initialized - Voltage: %.2fV, Percentage: %d%%", voltage, percentage);
#else
    ESP_LOGW(TAG, "Battery monitoring not available on this board");
#endif
}

bool BatteryManager::isAvailable() {
#ifdef BOARD_ESP32_S3
    return batteryInitialized;
#else
    return false;
#endif
}

float BatteryManager::getBatteryVoltage() {
#ifdef BOARD_ESP32_S3
    if (!batteryInitialized) {
        ESP_LOGW(TAG, "Battery manager not initialized");
        return -1.0f;
    }

    // Enable ADC circuit
    digitalWrite(ADC_EN_PIN, HIGH);
    delay(10); // Short delay to stabilize

    // Read ADC value multiple times and average to reduce noise
    uint32_t adcSum = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        adcSum += analogRead(BATTERY_ADC_PIN);
        delayMicroseconds(100); // Small delay between readings
    }
    float adcValue = adcSum / (float)BATTERY_SAMPLES;

    // Disable ADC circuit to save power
    digitalWrite(ADC_EN_PIN, LOW);

    // Convert ADC value to voltage
    // Formula from OG DIY Kit: (ADC / 4095.0) * 3.6V * 2.0 (divider) * calibration
    float voltage = (adcValue / ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE * VOLTAGE_DIVIDER_RATIO * CALIBRATION_FACTOR;

    ESP_LOGD(TAG, "Battery ADC: %.0f, Voltage: %.2fV", adcValue, voltage);

    return voltage;
#else
    return -1.0f;
#endif
}

int BatteryManager::getBatteryPercentage() {
#ifdef BOARD_ESP32_S3
    float voltage = getBatteryVoltage();
    if (voltage < 0) {
        return -1;
    }

    return (int)(voltageToPercentage(voltage));
#else
    return -1;
#endif
}

int BatteryManager::getBatteryIconLevel() {
#ifdef BOARD_ESP32_S3
    int percentage = getBatteryPercentage();
    if (percentage < 0) {
        return 0; // Not available
    }

    // Map percentage to icon level (1-5)
    if (percentage >= 80) return 5; // Battery_5: 80-100%
    if (percentage >= 60) return 4; // Battery_4: 60-79%
    if (percentage >= 40) return 3; // Battery_3: 40-59%
    if (percentage >= 20) return 2; // Battery_2: 20-39%
    return 1; // Battery_1: 0-19%
#else
    return 0;
#endif
}

bool BatteryManager::isCharging() {
#ifdef BOARD_ESP32_S3
    // Check if battery voltage is above fully charged threshold
    // This is a simple heuristic - voltage > 4.2V indicates charging
    float voltage = getBatteryVoltage();
    if (voltage < 0) {
        return false;
    }

    // If voltage is significantly above max, likely charging
    return voltage > (BATTERY_VOLTAGE_MAX + 0.1f);
#else
    return false;
#endif
}

float BatteryManager::voltageToPercentage(float voltage) {
    // Clamp voltage to valid range
    if (voltage >= BATTERY_VOLTAGE_MAX) {
        return 100.0f;
    }
    if (voltage <= BATTERY_VOLTAGE_MIN) {
        return 0.0f;
    }
    // Linear interpolation between min and max voltage
    // Note: LiPo discharge curve is not perfectly linear, but this is a reasonable approximation
    float percentage = ((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0f;

    // Ensure percentage is within bounds
    if (percentage > 100.0f) percentage = 100.0f;
    if (percentage < 0.0f) percentage = 0.0f;

    return percentage;
}

