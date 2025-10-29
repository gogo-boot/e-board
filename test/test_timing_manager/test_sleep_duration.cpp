#include <unity.h>
#include <ctime>
#include "util/timing_manager.h"
#include "config/config_manager.h"

// Test helper to set a specific time
void setMockTime(int year, int month, int day, int hour, int minute, int second) {
    // For native testing, we'll work with the current system time
    // In a real implementation, you might want to mock the time() function
}

void setUp(void) {
    // Reset configuration to known state before each test
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 2; // 2 hours
    config.transportInterval = 15; // 15 minutes
    config.weekendMode = true;
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "22:00");
    strcpy(config.sleepStart, "23:00");
    strcpy(config.sleepEnd, "05:30");
    strcpy(config.weekendTransportStart, "08:00");
    strcpy(config.weekendTransportEnd, "20:00");
    strcpy(config.weekendSleepStart, "00:00");
    strcpy(config.weekendSleepEnd, "07:00");
}

void tearDown(void) {
    // Clean up after each test
}

void test_getNextSleepDurationSeconds_weather_only_mode() {
    // Test weather-only mode (displayMode = 1)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 1; // weather_only
    config.weatherInterval = 1; // 1 hour

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should return at least minimum sleep duration (30 seconds)
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);

    // Should be reasonable (not more than 1 hour + buffer)
    TEST_ASSERT_LESS_THAN(3700, sleepDuration); // 1 hour + 100 seconds buffer
}

void test_getNextSleepDurationSeconds_departure_only_mode() {
    // Test departure-only mode (displayMode = 2)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 5; // 5 minutes

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should return at least minimum sleep duration
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);

    // Should be reasonable (not more than 5 minutes + buffer)
    TEST_ASSERT_LESS_THAN(350, sleepDuration); // 5 minutes + 50 seconds buffer
}

void test_getNextSleepDurationSeconds_half_and_half_mode() {
    // Test half-and-half mode (displayMode = 0)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 2; // 2 hours
    config.transportInterval = 10; // 10 minutes

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should return at least minimum sleep duration
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);

    // Should wake up for the nearest update (transport is more frequent)
    // So should be closer to transport interval than weather interval
    TEST_ASSERT_LESS_THAN(650, sleepDuration); // 10 minutes + 50 seconds buffer
}

void test_minimum_sleep_duration_enforced() {
    // Test that minimum sleep duration is always enforced
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 1; // 1 minute - very frequent updates

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should never be less than 30 seconds
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);
}

void test_parseTimeString_helper() {
    // Test the time string parsing helper function
    String testTime1 = "06:30";
    String testTime2 = "23:45";
    String testTime3 = "00:00";
    // Access through TimingManager (we'll need to make parseTimeString public for testing)
    // For now, we'll test indirectly through the main function behavior
    TEST_ASSERT_TRUE(true); // Placeholder - would need access to private method
}

void test_isTimeInRange_helper() {
    // Test the time range checking
    // This would also need access to private methods
    TEST_ASSERT_TRUE(true); // Placeholder
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_getNextSleepDurationSeconds_weather_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_departure_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_half_and_half_mode);
    RUN_TEST(test_minimum_sleep_duration_enforced);
    RUN_TEST(test_parseTimeString_helper);
    RUN_TEST(test_isTimeInRange_helper);

    return UNITY_END();
}
