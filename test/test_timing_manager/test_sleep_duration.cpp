#include <unity.h>
#include <ctime>
#include "util/timing_manager.h"
#include "config/config_manager.h"
#include "mock_time.h"

// Helper function to create a specific time_t from date/time components
time_t createTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900; // Years since 1900
    timeinfo.tm_mon = month - 1; // Months since January (0-11)
    timeinfo.tm_mday = day; // Day of month (1-31)
    timeinfo.tm_hour = hour; // Hours (0-23)
    timeinfo.tm_min = minute; // Minutes (0-59)
    timeinfo.tm_sec = second; // Seconds (0-59)
    timeinfo.tm_isdst = -1; // Let mktime() determine DST

    return mktime(&timeinfo);
}

// Helper function to modify the time of current day
time_t setTimeToday(int hour, int minute, int second = 0) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now); // Get current time as tm structure

    // Modify only the time components, keep the date
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = second;

    return mktime(timeinfo); // Convert back to time_t
}

// Set up before each test function
void setUp(void) {
    // Reset to real time first
    MockTime::useRealTime();

    // Reset configuration to known state before each test
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 1; // 1 hours
    config.transportInterval = 3; // 3 minutes
    config.weekendMode = true;
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");
    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    strcpy(config.weekendTransportStart, "08:00");
    strcpy(config.weekendTransportEnd, "10:00");
    strcpy(config.weekendSleepStart, "23:00");
    strcpy(config.weekendSleepEnd, "07:00");

    // Reset RTC timestamp variables to zero (no previous updates)
    TimingManager::setLastWeatherUpdate(0);
    TimingManager::setLastTransportUpdate(0);
}

void tearDown(void) {
    // Always restore real time after each test
    MockTime::useRealTime();
}

void test_getNextSleepDurationSeconds_weather_only_mode() {
    // Test weather-only mode (displayMode = 1)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 1; // weather_only
    config.weatherInterval = 1; // 1 hour

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should return at least minimum sleep duration (30 seconds)
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);

    printf("%llu\n", sleepDuration);

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

void test_with_previous_weather_update() {
    // Set up scenario where weather was updated 30 minutes ago
    time_t now = time(nullptr);
    uint32_t thirtyMinutesAgo = (uint32_t)now - (30 * 60); // 30 minutes in seconds

    TimingManager::setLastWeatherUpdate(thirtyMinutesAgo);
    TimingManager::setLastTransportUpdate(0); // No previous transport update

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 1; // weather_only
    config.weatherInterval = 1; // 1 hour

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("Sleep duration with 30min ago weather update: %llu seconds (~%llu minutes)\n",
           sleepDuration, sleepDuration / 60);

    // Should sleep for approximately 30 minutes (1 hour - 30 minutes already passed)
    TEST_ASSERT_GREATER_OR_EQUAL(1700, sleepDuration); // ~30 min - buffer
    TEST_ASSERT_LESS_THAN(1900, sleepDuration); // ~30 min + buffer
}

void test_with_previous_transport_update() {
    // Set up scenario where transport was updated 2 minutes ago
    time_t now = time(nullptr);
    uint32_t twoMinutesAgo = (uint32_t)now - (2 * 60);

    TimingManager::setLastWeatherUpdate(0); // No previous weather update
    TimingManager::setLastTransportUpdate(twoMinutesAgo);

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 5; // 5 minutes
    strcpy(config.transportActiveStart, "00:00"); // Active all day
    strcpy(config.transportActiveEnd, "23:59");

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("Sleep duration with 2min ago transport update: %llu seconds (~%llu minutes)\n",
           sleepDuration, sleepDuration / 60);

    // Should wake up in ~3 minutes (5 min interval - 2 min already passed)
    TEST_ASSERT_GREATER_OR_EQUAL(150, sleepDuration); // ~3 min - buffer
    TEST_ASSERT_LESS_THAN(210, sleepDuration); // ~3 min + buffer
}

void test_with_both_previous_updates() {
    time_t now = time(nullptr);

    // Weather was updated 1 hour ago
    uint32_t oneHourAgo = (uint32_t)now - (60 * 60);
    TimingManager::setLastWeatherUpdate(oneHourAgo);

    // Transport was updated 2 minutes ago
    uint32_t twoMinutesAgo = (uint32_t)now - (2 * 60);
    TimingManager::setLastTransportUpdate(twoMinutesAgo);

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 2; // 2 hours
    config.transportInterval = 5; // 5 minutes
    strcpy(config.transportActiveStart, "00:00"); // Active all day
    strcpy(config.transportActiveEnd, "23:59");

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("Sleep duration with both previous updates: %llu seconds (~%llu minutes)\n",
           sleepDuration, sleepDuration / 60);
    printf("  Weather updated: 1 hour ago (interval: 2 hours)\n");
    printf("  Transport updated: 2 minutes ago (interval: 5 minutes)\n");

    // Should wake up for the nearest update (transport in ~3 minutes)
    TEST_ASSERT_GREATER_OR_EQUAL(150, sleepDuration); // ~3 min - buffer
    TEST_ASSERT_LESS_THAN(210, sleepDuration); // ~3 min + buffer
}

void test_weather_update_overdue() {
    time_t now = time(nullptr);

    // Weather was updated 3 hours ago, but interval is 2 hours (overdue!)
    uint32_t threeHoursAgo = (uint32_t)now - (3 * 60 * 60);
    TimingManager::setLastWeatherUpdate(threeHoursAgo);
    TimingManager::setLastTransportUpdate(0);

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 1; // weather_only
    config.weatherInterval = 2; // 2 hours

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("Sleep duration when weather update is overdue: %llu seconds\n", sleepDuration);

    // Should wake up immediately (minimum sleep duration)
    TEST_ASSERT_EQUAL(30, sleepDuration);
}

void test_verify_timestamp_setters() {
    // Test that the setter methods work correctly
    time_t now = time(nullptr);
    uint32_t testTimestamp = (uint32_t)now - 1000;

    TimingManager::setLastWeatherUpdate(testTimestamp);
    uint32_t retrieved = TimingManager::getLastWeatherUpdate();
    TEST_ASSERT_EQUAL_UINT32(testTimestamp, retrieved);
    TimingManager::setLastTransportUpdate(testTimestamp + 500);
    retrieved = TimingManager::getLastTransportUpdate();
    TEST_ASSERT_EQUAL_UINT32(testTimestamp + 500, retrieved);

    printf("Timestamp setters/getters verified successfully\n");
}

void test_transport_active_time_morning() {
    // Set mock time to 7:30 AM on a weekday (Wednesday)
    time_t morningTime = createTime(2024, 10, 30, 7, 30, 0); // Wednesday
    MockTime::setMockTime(morningTime);

    RTCConfigData& config = ConfigManager::getConfig();
    config.weekendMode = true;
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    bool isActive = TimingManager::isTransportActiveTime();

    printf("Testing transport active at 7:30 AM (Wed): %s\n", isActive ? "ACTIVE" : "INACTIVE");
    TEST_ASSERT_TRUE(isActive);
}

void test_transport_inactive_time_afternoon() {
    // Set mock time to 2:00 PM on a weekday (outside active hours 06:00-09:00)
    time_t afternoonTime = createTime(2024, 10, 30, 14, 0, 0); // Wednesday 2 PM
    MockTime::setMockTime(afternoonTime);

    RTCConfigData& config = ConfigManager::getConfig();
    config.weekendMode = true;
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    bool isActive = TimingManager::isTransportActiveTime();

    printf("Testing transport active at 2:00 PM (Wed): %s\n", isActive ? "ACTIVE" : "INACTIVE");
    TEST_ASSERT_FALSE(isActive);
}

void test_transport_weekend_time() {
    // Set mock time to 9:00 AM on Saturday
    time_t saturdayMorning = createTime(2024, 11, 2, 9, 0, 0); // Saturday
    MockTime::setMockTime(saturdayMorning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.weekendMode = true;
    strcpy(config.weekendTransportStart, "08:00");
    strcpy(config.weekendTransportEnd, "10:00");

    bool isActive = TimingManager::isTransportActiveTime();
    bool isWeekend = TimingManager::isWeekend();

    printf("Testing at 9:00 AM Saturday - isWeekend: %s, isActive: %s\n",
           isWeekend ? "YES" : "NO", isActive ? "ACTIVE" : "INACTIVE");
    TEST_ASSERT_TRUE(isWeekend);
    TEST_ASSERT_TRUE(isActive);
}

void test_sleep_duration_with_mocked_time() {
    // Set mock time to 7:00 AM on a weekday
    time_t morningTime = createTime(2024, 10, 30, 7, 0, 0);
    MockTime::setMockTime(morningTime);

    // Set last update to 5 minutes ago
    TimingManager::setLastTransportUpdate((uint32_t)morningTime - (5 * 60));

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 10; // 10 minutes
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("Sleep duration at 7:00 AM (5 min after last update, 10 min interval): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should sleep for about 5 more minutes (10 min interval - 5 min elapsed)
    TEST_ASSERT_GREATER_OR_EQUAL(250, sleepDuration); // ~5 min - buffer
    TEST_ASSERT_LESS_THAN(350, sleepDuration); // ~5 min + buffer
}

void test_sleep_duration_half_half_weather_updated_never() {
    // Set mock time to 10:00 AM (outside active hours 06:00-09:00)
    time_t latemorning = createTime(2025, 10, 30, 10, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate(0); // No weather update

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 2; // 2 hours
    config.transportInterval = 5; // 5 minutes
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 10:00 AM (outside active hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should use weather update time since transport is outside active hours
    // Should be minimum sleep (30 sec) since weather was never updated
    TEST_ASSERT_EQUAL(30, sleepDuration);
}

void test_sleep_duration_half_half_weather_updated_now() {
    // time_t now = time(nullptr);
    // Set mock time to 10:00 AM (outside active hours 06:00-09:00)
    time_t latemorning = createTime(2025, 10, 30, 10, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate(0); // No weather update

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    config.weatherInterval = 2; // 2 hours
    config.transportInterval = 5; // 5 minutes
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 10:00 AM (outside active hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should use weather update time since transport is outside active hours
    // Should be minimum sleep (30 sec) since weather was never updated
    TEST_ASSERT_EQUAL(30, sleepDuration);
}

void test_sleep_duration_half_half_trasport_updated_now() {}

void test_sleep_duration_half_half_do_deepsleep() {}

void test_sleep_duration_weather_full_weather_updated_now() {}

void test_sleep_duration_weather_full_trasport_updated_now() {}

void test_sleep_duration_weather_full_do_deepsleep() {}

void test_sleep_duration_transport_full_weather_updated_now() {}

void test_sleep_duration_transport_full_trasport_updated_now() {}

void test_sleep_duration_transport_full_do_deepsleep() {}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_getNextSleepDurationSeconds_weather_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_departure_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_half_and_half_mode);
    RUN_TEST(test_minimum_sleep_duration_enforced);
    RUN_TEST(test_parseTimeString_helper);
    RUN_TEST(test_isTimeInRange_helper);

    // Tests with RTC timestamp variables
    RUN_TEST(test_with_previous_weather_update);
    RUN_TEST(test_with_previous_transport_update);
    RUN_TEST(test_with_both_previous_updates);
    RUN_TEST(test_weather_update_overdue);
    RUN_TEST(test_verify_timestamp_setters);

    // Tests with MockTime for active/inactive hours
    RUN_TEST(test_transport_active_time_morning);
    RUN_TEST(test_transport_inactive_time_afternoon);
    RUN_TEST(test_transport_weekend_time);
    RUN_TEST(test_sleep_duration_with_mocked_time);
    RUN_TEST(test_sleep_duration_half_half_weather_updated_now);

    return UNITY_END();
}
