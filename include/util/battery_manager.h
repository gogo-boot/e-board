#pragma once
#include <Arduino.h>

/**
 * Battery Manager for TRMNL 7.5" (OG) DIY Kit with ESP32-S3
 *
 * Manages battery voltage reading and status reporting.
 * Only available on TRMNL OG DIY Kit with ESP32-S3.
 *
 * Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
 * - Battery voltage is read from GPIO1 (A0) - BAT_ADC pin
 * - GPIO6 (A5) - ADC_EN controls power to the ADC circuit (power saving)
 * - Voltage divider: 2:1 ratio
 * - Battery range: ~3.0V (empty) to ~4.2V (full)
 * - Calibration factor: 0.968 (from OG DIY Kit reference)
 */

// Battery voltage thresholds (for LiPo batteries)
static const float BATTERY_VOLTAGE_MAX = 4.2f; // Fully charged
static const float BATTERY_VOLTAGE_MIN = 3.0f; // Empty (safe cutoff)
static const float BATTERY_VOLTAGE_NOMINAL = 3.7f; // Nominal voltage

#ifdef BOARD_ESP32_S3
// TRMNL 7.5" (OG) DIY Kit specific configuration
// Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
static const float VOLTAGE_DIVIDER_RATIO = 2.0f; // 2:1 voltage divider
static const int ADC_MAX_VALUE = 4095; // 12-bit ADC
static const float ADC_REFERENCE_VOLTAGE = 3.6f; // Actual reference voltage for ESP32-S3
static const float CALIBRATION_FACTOR = 0.968f; // Calibration factor from OG DIY Kit
static bool batteryInitialized = false;
#endif

class BatteryManager {
public:
    /**
     * Initialize battery manager
     * Must be called once during setup
     */
    static void init();

    /**
     * Check if battery monitoring is available on this board
     * @return true if battery monitoring is supported
     */
    static bool isAvailable();

    /**
     * Read current battery voltage
     * @return Battery voltage in volts, or -1.0 if not available
     */
    static float getBatteryVoltage();

    /**
     * Get battery percentage (0-100)
     * @return Battery percentage, or -1 if not available
     */
    static int getBatteryPercentage();

    /**
     * Get battery level icon (1-5)
     * 1 = lowest (0-20%), 5 = highest (80-100%)
     * @return Battery icon level (1-5), or 0 if not available
     */
    static int getBatteryIconLevel();

    /**
     * Check if battery is charging
     * @return true if charging, false otherwise
     */
    static bool isCharging();

private:
    static float voltageToPercentage(float voltage);
    static const int BATTERY_SAMPLES = 10; // Number of samples for averaging
};

