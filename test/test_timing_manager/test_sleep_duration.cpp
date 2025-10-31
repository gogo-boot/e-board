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

// If weather-only mode, should wake up every hour
void test_getNextSleepDurationSeconds_weather_only_mode() {
    time_t morningTime = createTime(2025, 10, 30, 7, 30, 0); // Thursday
    MockTime::setMockTime(morningTime);

    // Test weather-only mode (displayMode = 1)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 1; // weather_only

    TimingManager::setLastWeatherUpdate((uint32_t)morningTime);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    TEST_ASSERT_EQUAL(3600, sleepDuration); // 1 hour
}

// If transport only mode, should wake up every 3 minutes during active hours
void test_getNextSleepDurationSeconds_departure_only_mode() {
    time_t morningTime = createTime(2025, 10, 30, 7, 30, 0); // Thursday
    MockTime::setMockTime(morningTime);

    // Test departure-only mode (displayMode = 2)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only

    TimingManager::setLastTransportUpdate((uint32_t)morningTime);
    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    TEST_ASSERT_EQUAL(180, sleepDuration);
}

// Todo - Failed.. fix it
// If transport only mode, but during inactive hours, should wake up in 21 hours after deep sleep
void test_getNextSleepDurationSeconds_departure_only_mode_inactive() {
    time_t morningTime = createTime(2025, 10, 30, 9, 0, 0); // Thursday
    MockTime::setMockTime(morningTime);

    // Test departure-only mode (displayMode = 2)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only

    TimingManager::setLastTransportUpdate((uint32_t)morningTime);
    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    TEST_ASSERT_EQUAL(3600 * 21, sleepDuration);
}

// If weather and transport not updated, Ensure minimum sleep duration of 30 seconds is enforced
void test_minimum_sleep_duration_enforced() {
    time_t morningTime = createTime(2025, 10, 30, 9, 0, 0); // Thursday
    MockTime::setMockTime(morningTime);

    // Test that minimum sleep duration is always enforced
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 1; // 1 minute - very frequent updates

    TimingManager::setLastWeatherUpdate(0);
    TimingManager::setLastTransportUpdate(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should never be less than 30 seconds
    TEST_ASSERT_GREATER_OR_EQUAL(30, sleepDuration);
}

// If transport was updated 2 minutes ago, should wake up in ~3 minutes for next update
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

// If weather was updated 1 hour ago and transport 2 minutes ago, should wake up for nearest update
void test_with_both_previous_updates() {
    time_t now = createTime(2025, 10, 30, 9, 0, 0);
    MockTime::setMockTime(now);

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
    TEST_ASSERT_EQUAL(180, sleepDuration); // ~3 min + buffer
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
    time_t morningTime = createTime(2024, 10, 30, 7, 30, 0); // Thursday
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

// Todo - Fix
// Friday night to Saturday morning deep sleep test
void test_friday_to_saturday_deepsleep() {
    // Set mock time to 9:00 AM on Saturday
    time_t fridayNight = createTime(2025, 10, 31, 22, 0, 0); // Friday night
    MockTime::setMockTime(fridayNight);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastWeatherUpdate((uint32_t)fridayNight);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep until Saturday 7 AM (transport inactive hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(3600 * 9, sleepDuration);
}

// Todo - Fix
// Sunday night to Monday morning deep sleep test
void test_sunday_to_monday_deepsleep() {
    // Set mock time to 9:00 AM on Saturday
    time_t sundayNight = createTime(2025, 11, 2, 22, 30, 0); // Sunday night
    MockTime::setMockTime(sundayNight);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastWeatherUpdate((uint32_t)sundayNight);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep until Monday 5:30 AM (transport inactive hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(3600 * 7, sleepDuration);
}

// If weather is never updated, transport just updated, during transport inactive hours
void test_sleep_duration_half_half_weather_updated_never() {
    time_t latemorning = createTime(2025, 10, 30, 10, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate(0); // No weather update

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 10:00 AM (outside active hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(30, sleepDuration);
}

// If both weather and transport just updated, during transport active hours
void test_sleep_duration_half_half_weather_transport_active_updated_now() {
    time_t latemorning = createTime(2025, 10, 30, 6, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate((uint32_t)latemorning);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 6:00 AM (active hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(180, sleepDuration);
}

// If both weather and transport just updated, during transport inactive hours
void test_sleep_duration_half_half_weather_transport_inactive_updated_now() {
    time_t latemorning = createTime(2025, 10, 30, 9, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate((uint32_t)latemorning);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 9:00 AM (transport inactive hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(3600, sleepDuration);
}

// Todo - Fix test
// If weather transport never updated, during transport inactive hours
void test_sleep_duration_half_half_weather_trasport_inactive_transport_not_updated() {
    time_t latemorning = createTime(2025, 10, 30, 9, 0, 0);
    MockTime::setMockTime(latemorning);

    // Set last transport update to now (just updated)
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastWeatherUpdate((uint32_t)latemorning);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 9:00 AM (transport inactive hours): %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    TEST_ASSERT_EQUAL(3600, sleepDuration);
}

// If weather updated and transport updated long ago before deep sleep period
void test_sleep_duration_half_half_deep_sleep() {
    time_t morning = createTime(2025, 10, 30, 9, 0, 0);
    time_t evening = createTime(2025, 10, 30, 22, 0, 0);
    MockTime::setMockTime(evening);

    TimingManager::setLastTransportUpdate((uint32_t)morning); // Updated at 09:00 AM
    TimingManager::setLastWeatherUpdate((uint32_t)evening); // Updated at 22:00 PM

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("Sleep duration at 22:00 AM : %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should sleep until 05:30 next day
    TEST_ASSERT_EQUAL(3600 * 7.5L, sleepDuration);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_getNextSleepDurationSeconds_weather_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_departure_only_mode);
    RUN_TEST(test_getNextSleepDurationSeconds_departure_only_mode_inactive);
    RUN_TEST(test_minimum_sleep_duration_enforced);

    // Tests with RTC timestamp variables
    RUN_TEST(test_with_previous_transport_update);
    RUN_TEST(test_with_both_previous_updates);
    RUN_TEST(test_weather_update_overdue);
    RUN_TEST(test_verify_timestamp_setters);

    // Tests with MockTime for active/inactive hours
    RUN_TEST(test_transport_active_time_morning);
    RUN_TEST(test_transport_inactive_time_afternoon);
    RUN_TEST(test_transport_weekend_time);
    RUN_TEST(test_friday_to_saturday_deepsleep);
    RUN_TEST(test_sunday_to_monday_deepsleep);

    // test start
    RUN_TEST(test_sleep_duration_half_half_weather_updated_never);
    RUN_TEST(test_sleep_duration_half_half_weather_transport_active_updated_now);
    RUN_TEST(test_sleep_duration_half_half_weather_transport_inactive_updated_now);
    RUN_TEST(test_sleep_duration_half_half_weather_trasport_inactive_transport_not_updated);
    RUN_TEST(test_sleep_duration_half_half_deep_sleep);

    return UNITY_END();
}
