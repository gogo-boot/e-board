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

    // OTA configuration
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00");

    // Reset RTC timestamp variables to zero (no previous updates)
    TimingManager::setLastWeatherUpdate(0);
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastOTACheck(0);
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

// If transport only mode, but during inactive hours, should wake up in 21 hours after deep sleep
void test_getNextSleepDurationSeconds_departure_only_mode_inactive() {
    time_t morningTime = createTime(2025, 10, 30, 9, 0, 0); // Thursday
    MockTime::setMockTime(morningTime);

    // Test departure-only mode (displayMode = 2)
    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.otaEnabled = false; // Disable OTA for this test

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
    // Use a fixed time during the day (not sleep period) for predictable results
    time_t now = createTime(2025, 10, 30, 10, 0, 0); // Thursday 10:00 AM
    MockTime::setMockTime(now);

    // Set up scenario where transport was updated 2 minutes ago
    uint32_t twoMinutesAgo = (uint32_t)now - (2 * 60);

    TimingManager::setLastWeatherUpdate(0); // No previous weather update
    TimingManager::setLastTransportUpdate(twoMinutesAgo);

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 5; // 5 minutes
    strcpy(config.transportActiveStart, "00:00"); // Active all day
    strcpy(config.transportActiveEnd, "23:59");
    strcpy(config.sleepStart, "22:30"); // Ensure we're not in sleep period
    strcpy(config.sleepEnd, "05:30");
    config.otaEnabled = false; // Disable OTA for this test

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

// ============================================================================
// OTA Update Tests
// ============================================================================

// Test: OTA disabled - should not affect sleep calculations
void test_ota_disabled() {
    time_t morning = createTime(2025, 10, 30, 2, 30, 0); // 2:30 AM (30 min before OTA time)
    MockTime::setMockTime(morning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = false; // Disable OTA
    strcpy(config.otaCheckTime, "03:00");

    TimingManager::setLastWeatherUpdate((uint32_t)morning);
    TimingManager::setLastTransportUpdate((uint32_t)morning);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA disabled - sleep duration: %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should wake for after deep sleep at 5:30, not OTA
    TEST_ASSERT_EQUAL(3600 * 3, sleepDuration);
}

// Test: OTA enabled and scheduled before next weather/transport update
void test_ota_enabled_before_other_updates() {
    time_t earlyMorning = createTime(2025, 10, 30, 2, 45, 0); // 2:45 AM (15 min before OTA)
    MockTime::setMockTime(earlyMorning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00");
    config.weatherInterval = 3; // 3 hours

    TimingManager::setLastWeatherUpdate((uint32_t)earlyMorning); // Weather just updated
    TimingManager::setLastTransportUpdate((uint32_t)earlyMorning); // Transport just updated
    TimingManager::setLastOTACheck(0); // No previous OTA check

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA check in 15 minutes - sleep duration: %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should wake in 15 minutes for OTA (900 seconds)
    TEST_ASSERT_EQUAL(900, sleepDuration);
}

// Test: OTA scheduled during deep sleep period - should bypass sleep
void test_ota_during_deep_sleep_bypasses_sleep() {
    time_t lateNight = createTime(2025, 10, 30, 23, 0, 0); // 11:00 PM (deep sleep period)
    MockTime::setMockTime(lateNight);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00"); // OTA at 3:00 AM (during sleep 22:30 - 05:30)
    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weatherInterval = 10; // 10 hours (won't wake for weather)

    TimingManager::setLastWeatherUpdate((uint32_t)lateNight);
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA at 3 AM during sleep - sleep duration: %llu seconds (~%llu hours)\n",
           sleepDuration, sleepDuration / 3600);

    // Should wake at 3:00 AM for OTA (4 hours = 14400 seconds), not at sleep end (5:30 AM)
    TEST_ASSERT_EQUAL(4 * 3600, sleepDuration);
}

// Test: OTA check already performed recently - should skip
void test_ota_already_checked_recently() {
    time_t morning = createTime(2025, 10, 30, 3, 0, 0); // Exactly OTA time
    MockTime::setMockTime(morning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00");

    // Set OTA check to 1 minute ago (should skip)
    TimingManager::setLastOTACheck((uint32_t)morning - 59);
    TimingManager::setLastWeatherUpdate((uint32_t)morning);
    TimingManager::setLastTransportUpdate((uint32_t)morning);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA already checked 1 min ago - sleep duration: %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should not schedule OTA again, wake for transport in 3 minutes
    TEST_ASSERT_EQUAL(3600 * 2.5, sleepDuration);
}

// Test: OTA scheduled later today
void test_ota_scheduled_later_today() {
    time_t morning = createTime(2025, 10, 30, 1, 0, 0); // 1:00 AM
    MockTime::setMockTime(morning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00"); // OTA at 3:00 AM (2 hours later)
    config.weatherInterval = 10; // 10 hours (won't interfere)

    TimingManager::setLastWeatherUpdate((uint32_t)morning);
    TimingManager::setLastTransportUpdate((uint32_t)morning);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA in 2 hours - sleep duration: %llu seconds (~%llu hours)\n",
           sleepDuration, sleepDuration / 3600);

    // Should wake in 2 hours for OTA
    TEST_ASSERT_EQUAL(2 * 3600, sleepDuration);
}

// Test: OTA scheduled tomorrow (past today's OTA time)
void test_ota_scheduled_tomorrow() {
    time_t afternoon = createTime(2025, 10, 30, 15, 0, 0); // 3:00 PM
    MockTime::setMockTime(afternoon);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00"); // Already passed today, next is tomorrow
    config.weatherInterval = 1; // 1 hour
    config.displayMode = 1; // weather only

    TimingManager::setLastWeatherUpdate((uint32_t)afternoon);
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA tomorrow at 3 AM - sleep duration: %llu seconds (~%llu hours)\n",
           sleepDuration, sleepDuration / 3600);

    // Should wake in 1 hour for weather (sooner than tomorrow's OTA at 12 hours)
    TEST_ASSERT_EQUAL(3600, sleepDuration);
}

// Test: OTA and weather update at same approximate time
void test_ota_and_weather_coincide() {
    time_t earlyMorning = createTime(2025, 10, 30, 2, 0, 0); // 2:00 AM
    MockTime::setMockTime(earlyMorning);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00"); // OTA at 3:00 AM
    config.weatherInterval = 1; // 1 hour
    config.displayMode = 1; // weather only

    // Weather was updated at 2:00 AM, next update at 3:00 AM (same as OTA)
    TimingManager::setLastWeatherUpdate((uint32_t)earlyMorning);
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA and weather both at 3 AM - sleep duration: %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should wake in 1 hour (both OTA and weather scheduled)
    TEST_ASSERT_EQUAL(3600, sleepDuration);
}

// Test: OTA during transport inactive hours (should still wake)
void test_ota_during_transport_inactive_hours() {
    time_t lateNight = createTime(2025, 10, 30, 2, 30, 0); // 2:30 AM (transport inactive)
    MockTime::setMockTime(lateNight);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00");
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");
    config.displayMode = 2; // departure only
    config.weatherInterval = 10; // won't interfere

    TimingManager::setLastWeatherUpdate(0);
    TimingManager::setLastTransportUpdate((uint32_t)lateNight);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();
    printf("OTA at 3 AM (transport inactive) - sleep duration: %llu seconds (~%llu min)\n",
           sleepDuration, sleepDuration / 60);

    // Should wake in 30 minutes for OTA, not wait until transport active hours
    TEST_ASSERT_EQUAL(30 * 60, sleepDuration);
}

// Test: OTA on weekend
void test_ota_on_weekend() {
    time_t saturdayNight = createTime(2025, 11, 1, 23, 0, 0); // Saturday 11:00 PM
    MockTime::setMockTime(saturdayNight);

    RTCConfigData& config = ConfigManager::getConfig();
    config.otaEnabled = true;
    strcpy(config.otaCheckTime, "03:00");
    config.weekendMode = true;
    strcpy(config.weekendSleepStart, "23:00");
    strcpy(config.weekendSleepEnd, "07:00");
    config.weatherInterval = 10;

    TimingManager::setLastWeatherUpdate((uint32_t)saturdayNight);
    TimingManager::setLastTransportUpdate(0);
    TimingManager::setLastOTACheck(0);

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    printf("OTA at 3 AM on Sunday (weekend sleep) - sleep duration: %llu seconds (~%llu hours)\n",
           sleepDuration, sleepDuration / 3600);

    // Should wake at 3:00 AM for OTA (4 hours), not at weekend sleep end (7:00 AM)
    TEST_ASSERT_EQUAL(4 * 3600, sleepDuration);
}

// Test: Verify OTA timestamp getters/setters
void test_ota_timestamp_management() {
    time_t now = time(nullptr);
    uint32_t testTimestamp = (uint32_t)now - 500;

    TimingManager::setLastOTACheck(testTimestamp);
    uint32_t retrieved = TimingManager::getLastOTACheck();

    TEST_ASSERT_EQUAL_UINT32(testTimestamp, retrieved);
    printf("OTA timestamp setters/getters verified successfully\n");
}

// ============================================================================
// TEMPORARY MODE TESTS
// ============================================================================

// Test: Temporary mode during active hours - weather only mode
void test_temp_mode_active_hours_weather_only() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 10:00 AM Wednesday (active hours)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10; // November (0-based)
    testTime.tm_mday = 26; // Wednesday
    testTime.tm_hour = 10;
    testTime.tm_min = 0;
    testTime.tm_sec = 0;
    time_t testTimestamp = mktime(&testTime);
    MockTime::setMockTime(testTimestamp);

    // Activate temporary mode for weather-only
    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
    config.temporaryModeActivationTime = (uint32_t)testTimestamp;
    config.displayMode = DISPLAY_MODE_HALF_AND_HALF; // Configured mode is different

    // Sleep period: 22:30 - 05:30
    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should sleep for 2 minutes (120 seconds)
    TEST_ASSERT_EQUAL_UINT64(120, sleepDuration);
    printf("Temp mode (active hours, just activated): %llu seconds\n", sleepDuration);

    // Cleanup
    config.inTemporaryMode = false;
}

// Test: Temporary mode with elapsed time
void test_temp_mode_active_hours_elapsed_time() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 10:01:30 AM Wednesday (1 minute 30 seconds after activation)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10;
    testTime.tm_mday = 26;
    testTime.tm_hour = 10;
    testTime.tm_min = 1;
    testTime.tm_sec = 30;
    time_t currentTimestamp = mktime(&testTime);
    MockTime::setMockTime(currentTimestamp);

    // Activation time was 1:30 ago
    time_t activationTime = currentTimestamp - 90; // 90 seconds ago

    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
    config.temporaryModeActivationTime = (uint32_t)activationTime;
    config.displayMode = DISPLAY_MODE_DEPARTURE_ONLY;

    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should sleep for remaining 30 seconds (120 - 90 = 30)
    TEST_ASSERT_EQUAL_UINT64(30, sleepDuration);
    printf("Temp mode (1:30 elapsed, 30s remaining): %llu seconds\n", sleepDuration);

    config.inTemporaryMode = false;
}

// Test: Temporary mode during deep sleep period
void test_temp_mode_during_deep_sleep() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 1:00 AM Thursday (during deep sleep 22:30 - 05:30)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10;
    testTime.tm_mday = 27;
    testTime.tm_hour = 1;
    testTime.tm_min = 0;
    testTime.tm_sec = 0;
    time_t testTimestamp = mktime(&testTime);
    MockTime::setMockTime(testTimestamp);

    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_DEPARTURE_ONLY;
    config.temporaryModeActivationTime = (uint32_t)testTimestamp;
    config.displayMode = DISPLAY_MODE_WEATHER_ONLY;

    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should sleep until 05:30 (4.5 hours = 270 minutes = 16200 seconds)
    uint64_t expectedSleep = (5 * 60 + 30) * 60; // 05:30 in seconds from midnight
    uint64_t currentMinutes = 1 * 60; // 01:00 in minutes from midnight
    uint64_t expectedDuration = (expectedSleep / 60 - currentMinutes) * 60;

    TEST_ASSERT_EQUAL_UINT64(expectedDuration, sleepDuration);
    printf("Temp mode (during deep sleep, stay until 05:30): %llu seconds (~%.1f hours)\n",
           sleepDuration, sleepDuration / 3600.0);

    config.inTemporaryMode = false;
}

// Test: Temporary mode activated before deep sleep starts
void test_temp_mode_before_deep_sleep_starts() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 22:58 Wednesday (2 minutes before deep sleep at 23:00)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10;
    testTime.tm_mday = 26;
    testTime.tm_hour = 22;
    testTime.tm_min = 58;
    testTime.tm_sec = 0;
    time_t testTimestamp = mktime(&testTime);
    MockTime::setMockTime(testTimestamp);

    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_HALF_AND_HALF;
    config.temporaryModeActivationTime = (uint32_t)testTimestamp;
    config.displayMode = DISPLAY_MODE_WEATHER_ONLY;

    strcpy(config.sleepStart, "23:00");
    strcpy(config.sleepEnd, "06:00");
    config.weekendMode = false;

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should sleep for remaining 2 minutes until 2-minute temp mode completes
    // Then on next wake, will be in deep sleep period and stay until 06:00
    TEST_ASSERT_EQUAL_UINT64(120, sleepDuration);
    printf("Temp mode (before deep sleep): %llu seconds (2 min)\n", sleepDuration);

    config.inTemporaryMode = false;
}

// Test: Temporary mode exits after 2 minutes in active hours
void test_temp_mode_exit_after_2_minutes() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 10:02:00 AM (exactly 2 minutes after activation at 10:00:00)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10;
    testTime.tm_mday = 26;
    testTime.tm_hour = 10;
    testTime.tm_min = 2;
    testTime.tm_sec = 0;
    time_t currentTimestamp = mktime(&testTime);
    MockTime::setMockTime(currentTimestamp);

    time_t activationTime = currentTimestamp - 120; // Exactly 2 minutes ago

    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
    config.temporaryModeActivationTime = (uint32_t)activationTime;
    config.displayMode = DISPLAY_MODE_WEATHER_ONLY;
    config.weatherInterval = 1; // 1 hour

    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weekendMode = false;

    // Set last weather update to now so next update is in 1 hour
    TimingManager::setLastWeatherUpdate((uint32_t)currentTimestamp);

    // SIMULATE REAL BOOT FLOW: Button manager would clear the flag before timing manager runs
    // Since 2 minutes have elapsed, button manager clears the flag
    int elapsed = currentTimestamp - activationTime;
    if (elapsed >= 120) {
        config.inTemporaryMode = false;
        config.temporaryDisplayMode = 0xFF;
        config.temporaryModeActivationTime = 0;
    }

    // Now timing manager calculates sleep with flag already cleared
    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Temp mode should be exited, sleep until next weather update (1 hour)
    TEST_ASSERT_EQUAL_UINT64(3600, sleepDuration);

    // Verify temp mode was cleared (by button manager simulation above)
    TEST_ASSERT_FALSE(config.inTemporaryMode);
    TEST_ASSERT_EQUAL_UINT8(0xFF, config.temporaryDisplayMode);
    TEST_ASSERT_EQUAL_UINT32(0, config.temporaryModeActivationTime);

    printf("Temp mode exited after 2 min, sleep until next update: %llu seconds (1 hour)\n",
           sleepDuration);
}

// Test: Temp mode should be cleared after sleep calculation and persist across wake cycles
// This test reproduces the bug where temp mode flag is cleared but device still shows temp mode on next wake
void test_temp_mode_flag_persists_after_clearing() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Setup: 10:02:00 AM (2 minutes after temp mode activation)
    struct tm testTime = {0};
    testTime.tm_year = 2025 - 1900;
    testTime.tm_mon = 10;
    testTime.tm_mday = 26;
    testTime.tm_hour = 10;
    testTime.tm_min = 2;
    testTime.tm_sec = 0;
    time_t currentTimestamp = mktime(&testTime);
    MockTime::setMockTime(currentTimestamp);

    time_t activationTime = currentTimestamp - 120; // Exactly 2 minutes ago

    // FIRST WAKE CYCLE: Activate temp mode
    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
    config.temporaryModeActivationTime = (uint32_t)activationTime;
    config.displayMode = DISPLAY_MODE_HALF_AND_HALF; // Configured mode is different
    config.weatherInterval = 1; // 1 hour

    strcpy(config.sleepStart, "22:30");
    strcpy(config.sleepEnd, "05:30");
    config.weekendMode = false;

    TimingManager::setLastWeatherUpdate((uint32_t)currentTimestamp);

    // Simulate: Device checks temp mode flag BEFORE calling sleep calculator
    bool tempModeBeforeSleep = config.inTemporaryMode;
    uint8_t displayModeShown = tempModeBeforeSleep ? config.temporaryDisplayMode : config.displayMode;

    // Device displayed temp mode (weather-only)
    TEST_ASSERT_TRUE(tempModeBeforeSleep);
    TEST_ASSERT_EQUAL_UINT8(DISPLAY_MODE_WEATHER_ONLY, displayModeShown);

    // SIMULATE REAL BOOT FLOW: Button manager clears flag before timing manager
    // Since 2 minutes elapsed, button manager would clear the flag
    int elapsed = currentTimestamp - config.temporaryModeActivationTime;
    if (elapsed >= 120) {
        config.inTemporaryMode = false;
        config.temporaryDisplayMode = 0xFF;
        config.temporaryModeActivationTime = 0;
    }

    // Now calculate sleep (flag already cleared by button manager)
    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Verify temp mode was cleared (by button manager simulation above)
    TEST_ASSERT_FALSE_MESSAGE(config.inTemporaryMode,
                              "Temp mode should be cleared by button manager before sleep calculator runs");
    TEST_ASSERT_EQUAL_UINT8(0xFF, config.temporaryDisplayMode);
    TEST_ASSERT_EQUAL_UINT32(0, config.temporaryModeActivationTime);

    // Sleep duration should be next weather update (1 hour)
    TEST_ASSERT_EQUAL_UINT64(3600, sleepDuration);

    // SIMULATE SLEEP AND WAKE CYCLE
    // Advance time by sleep duration (1 hour)
    testTime.tm_hour = 11; // 11:02 AM
    testTime.tm_min = 2;
    time_t nextWakeTime = mktime(&testTime);
    MockTime::setMockTime(nextWakeTime);

    // SECOND WAKE CYCLE: Check temp mode flag again
    // This simulates what boot_flow_manager does on wake
    bool tempModeAfterWake = config.inTemporaryMode;
    uint8_t displayModeAfterWake = tempModeAfterWake ? config.temporaryDisplayMode : config.displayMode;

    // BUG CHECK: Temp mode should still be FALSE (not re-activated)
    TEST_ASSERT_FALSE_MESSAGE(tempModeAfterWake, "FAIL: Temp mode flag should remain FALSE after wake from sleep");

    // Should display configured mode (half-and-half), NOT temp mode (weather-only)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(DISPLAY_MODE_HALF_AND_HALF, displayModeAfterWake,
                                    "FAIL: Should display configured mode (half-and-half), not temp mode (weather-only)");

    printf("✓ Temp mode correctly cleared and persisted through wake cycle\n");
    printf("  - First wake: Showed temp mode (weather-only)\n");
    printf("  - Button manager: Cleared temp mode flag (2 min elapsed)\n");
    printf("  - Second wake: Showed configured mode (half-and-half)\n");
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

    // OTA update tests
    RUN_TEST(test_ota_disabled);
    RUN_TEST(test_ota_enabled_before_other_updates);
    RUN_TEST(test_ota_during_deep_sleep_bypasses_sleep);
    RUN_TEST(test_ota_already_checked_recently);
    RUN_TEST(test_ota_scheduled_later_today);
    RUN_TEST(test_ota_scheduled_tomorrow);
    RUN_TEST(test_ota_and_weather_coincide);
    RUN_TEST(test_ota_during_transport_inactive_hours);
    RUN_TEST(test_ota_on_weekend);
    RUN_TEST(test_ota_timestamp_management);

    // Temporary mode tests
    RUN_TEST(test_temp_mode_active_hours_weather_only);
    RUN_TEST(test_temp_mode_active_hours_elapsed_time);
    RUN_TEST(test_temp_mode_during_deep_sleep);
    RUN_TEST(test_temp_mode_before_deep_sleep_starts);
    RUN_TEST(test_temp_mode_exit_after_2_minutes);
    RUN_TEST(test_temp_mode_flag_persists_after_clearing); // TDD test for bug fix

    return UNITY_END();
}
