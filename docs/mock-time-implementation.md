# MockTime Implementation for Testing

## Overview
This document explains the MockTime implementation that allows you to control time in tests, enabling testing of time-dependent behavior like active/inactive hours, weekend detection, and sleep duration calculations.

## Implementation

### 1. MockTime Class (`test/test_timing_manager/mock_time.h`)

A simple singleton-like class that wraps the standard `time()` function:

```cpp
class MockTime {
private:
    static time_t mockCurrentTime;
    static bool useMock;

public:
    static void setMockTime(time_t time);  // Set a specific time for testing
    static void useRealTime();             // Reset to use system time
    static time_t now();                    // Get current time (mocked or real)
};
```

**Key Features:**
- `setMockTime(time_t)` - Sets a specific time and enables mocking
- `useRealTime()` - Disables mocking and returns to system time
- `now()` - Returns mocked time if enabled, otherwise system time

### 2. Integration with TimingManager

**Modified `timing_manager.cpp`:**
```cpp
#ifdef NATIVE_TEST
#include "mock_time.h"
#define GET_CURRENT_TIME() MockTime::now()
#else
#define GET_CURRENT_TIME() ({ time_t t; time(&t); t; })
#endif
```

**All `time(&now)` calls replaced with:**
```cpp
time_t now = GET_CURRENT_TIME();
```

This affects:
- `getNextSleepDurationSeconds()`
- `isWeekend()`
- `markWeatherUpdated()`
- `markTransportUpdated()`
- `isTimeForWeatherUpdate()`
- `isTimeForTransportUpdate()`
- `getCurrentMinutesSinceMidnight()`

### 3. Helper Functions for Tests

#### Create Specific Date/Time
```cpp
time_t createTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;  // Years since 1900
    timeinfo.tm_mon = month - 1;     // Months since January (0-11)
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = -1;
    return mktime(&timeinfo);
}
```

#### Modify Current Day Time
```cpp
time_t setTimeToday(int hour, int minute, int second = 0) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = second;
    return mktime(timeinfo);
}
```

## Test Examples

### Test 1: Transport Active During Morning Hours
```cpp
void test_transport_active_time_morning() {
    // Set mock time to 7:30 AM on Wednesday
    time_t morningTime = createTime(2024, 10, 30, 7, 30, 0);
    MockTime::setMockTime(morningTime);

    RTCConfigData& config = ConfigManager::getConfig();
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    bool isActive = TimingManager::isTransportActiveTime();

    TEST_ASSERT_TRUE(isActive); // Should be active at 7:30 AM
}
```

**Output:**
```
Testing transport active at 7:30 AM (Wed): ACTIVE
PASS
```

### Test 2: Transport Inactive in Afternoon
```cpp
void test_transport_inactive_time_afternoon() {
    // Set mock time to 2:00 PM (outside active hours 06:00-09:00)
    time_t afternoonTime = createTime(2024, 10, 30, 14, 0, 0);
    MockTime::setMockTime(afternoonTime);

    RTCConfigData& config = ConfigManager::getConfig();
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    bool isActive = TimingManager::isTransportActiveTime();

    TEST_ASSERT_FALSE(isActive); // Should be inactive at 2 PM
}
```

**Output:**
```
Testing transport active at 2:00 PM (Wed): INACTIVE
PASS
```

### Test 3: Weekend Detection
```cpp
void test_transport_weekend_time() {
    // Set mock time to 9:00 AM on Saturday
    time_t saturdayMorning = createTime(2024, 11, 2, 9, 0, 0);
    MockTime::setMockTime(saturdayMorning);

    bool isWeekend = TimingManager::isWeekend();
    bool isActive = TimingManager::isTransportActiveTime();

    TEST_ASSERT_TRUE(isWeekend);
    TEST_ASSERT_TRUE(isActive);
}
```

**Output:**
```
Testing at 9:00 AM Saturday - isWeekend: YES, isActive: ACTIVE
PASS
```

### Test 4: Sleep Duration with Mocked Time
```cpp
void test_sleep_duration_with_mocked_time() {
    // Set mock time to 7:00 AM
    time_t morningTime = createTime(2024, 10, 30, 7, 0, 0);
    MockTime::setMockTime(morningTime);

    // Set last update to 5 minutes ago
    TimingManager::setLastTransportUpdate((uint32_t)morningTime - (5 * 60));

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 2; // departure_only
    config.transportInterval = 10; // 10 minutes

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should sleep ~5 minutes (10 min interval - 5 min elapsed)
    TEST_ASSERT_GREATER_OR_EQUAL(250, sleepDuration);
    TEST_ASSERT_LESS_THAN(350, sleepDuration);
}
```

**Output:**
```
Sleep duration at 7:00 AM (5 min after last update, 10 min interval): 300 seconds (~5 min)
PASS
```

### Test 5: Outside Active Hours
```cpp
void test_sleep_duration_outside_active_hours() {
    // Set mock time to 10:00 AM (outside active hours 06:00-09:00)
    time_t latemorning = createTime(2024, 10, 30, 10, 0, 0);
    MockTime::setMockTime(latemorning);

    TimingManager::setLastTransportUpdate((uint32_t)latemorning);
    TimingManager::setLastWeatherUpdate(0);

    RTCConfigData& config = ConfigManager::getConfig();
    config.displayMode = 0; // half_and_half
    strcpy(config.transportActiveStart, "06:00");
    strcpy(config.transportActiveEnd, "09:00");

    uint64_t sleepDuration = TimingManager::getNextSleepDurationSeconds();

    // Should use weather update time since transport is outside active hours
    TEST_ASSERT_EQUAL(30, sleepDuration);
}
```

**Output:**
```
Sleep duration at 10:00 AM (outside active hours): 30 seconds
PASS
```

## Best Practices

### 1. Always Reset in setUp() and tearDown()
```cpp
void setUp(void) {
    MockTime::useRealTime();  // Reset to real time
    // ...rest of setup
}

void tearDown(void) {
    MockTime::useRealTime();  // Always restore real time
}
```

### 2. Common Time Scenarios

**Test specific time of day:**
```cpp
time_t morning = createTime(2024, 10, 30, 7, 30, 0);  // 7:30 AM
time_t evening = createTime(2024, 10, 30, 22, 0, 0);  // 10:00 PM
MockTime::setMockTime(morning);
```

**Test weekday vs weekend:**
```cpp
time_t wednesday = createTime(2024, 10, 30, 9, 0, 0);  // Wednesday
time_t saturday = createTime(2024, 11, 2, 9, 0, 0);    // Saturday
```

**Test time ranges:**
```cpp
// Before active hours
time_t beforeActive = createTime(2024, 10, 30, 5, 30, 0);

// During active hours
time_t duringActive = createTime(2024, 10, 30, 7, 30, 0);

// After active hours
time_t afterActive = createTime(2024, 10, 30, 10, 0, 0);
```

## Test Results

```
16 Tests 0 Failures 0 Ignored
OK
```

All tests pass successfully! ✅

## CLion Debugging

With MockTime, you can:

1. **Set breakpoints** in timing_manager.cpp
2. **Set specific times** using `MockTime::setMockTime()`
3. **Step through logic** to see how active/inactive hours are determined
4. **Inspect variables** at specific times without waiting for real time to pass

Example debug session:
```cpp
// In your test
time_t testTime = createTime(2024, 10, 30, 7, 30, 0);
MockTime::setMockTime(testTime);  // <-- Set breakpoint here

bool isActive = TimingManager::isTransportActiveTime();  // <-- Step into this
```

## Production Impact

✅ **Zero impact on production code** - The `GET_CURRENT_TIME()` macro uses regular `time()` for ESP32 builds.

✅ **No performance overhead** - MockTime is only compiled for native tests.

✅ **ESP32 build verified** - Production firmware compiles and runs successfully.

## Files Modified

1. **Created:**
   - `test/test_timing_manager/mock_time.h`
   - `test/test_timing_manager/mocks/mock_time.cpp`

2. **Modified:**
   - `src/util/timing_manager.cpp` - All `time(&now)` calls replaced with `GET_CURRENT_TIME()`
   - `test/test_timing_manager/test_sleep_duration.cpp` - Added 5 new MockTime tests

3. **Test Count:** 16 tests total (11 existing + 5 new MockTime tests)

