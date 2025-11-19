# Test Cases with RTC Timestamp Variables

## Overview

This document explains the test cases that set RTC timestamp variables (`lastWeatherUpdate` and `lastTransportUpdate`)
before running tests to verify the sleep duration calculation logic.

## Changes Made

### 1. Made RTC Timestamp Methods Public

**File:** `include/util/timing_manager.h`

Moved the following methods from `private` to `public` section to allow tests to set and verify timestamps:

```cpp
// RTC timestamp management (public for testing)
static uint32_t getLastWeatherUpdate();
static uint32_t getLastTransportUpdate();
static void setLastWeatherUpdate(uint32_t timestamp);
static void setLastTransportUpdate(uint32_t timestamp);
```

### 2. Updated setUp() Function

**File:** `test/test_timing_manager/test_sleep_duration.cpp`

Added reset of RTC timestamp variables in `setUp()`:

```cpp
void setUp(void) {
    // ...existing config setup...

    // Reset RTC timestamp variables to zero (no previous updates)
    TimingManager::setLastWeatherUpdate(0);
    TimingManager::setLastTransportUpdate(0);
}
```

## New Test Cases

### 1. `test_with_previous_weather_update()`

**Scenario:** Weather was updated 30 minutes ago

- Sets `lastWeatherUpdate` to 30 minutes ago
- Configures 1-hour weather interval
- **Expected:** Should sleep ~30 minutes (remaining time until next update)

```cpp
time_t now = time(nullptr);
uint32_t thirtyMinutesAgo = (uint32_t)now - (30 * 60);
TimingManager::setLastWeatherUpdate(thirtyMinutesAgo);
```

**Output:**

```
Sleep duration with 30min ago weather update: 1800 seconds (~30 minutes)
```

### 2. `test_with_previous_transport_update()`

**Scenario:** Transport was updated 2 minutes ago

- Sets `lastTransportUpdate` to 2 minutes ago
- Configures 5-minute transport interval
- **Expected:** Should sleep ~3 minutes (5 min - 2 min = 3 min remaining)

```cpp
time_t now = time(nullptr);
uint32_t twoMinutesAgo = (uint32_t)now - (2 * 60);
TimingManager::setLastTransportUpdate(twoMinutesAgo);
```

**Output:**

```
Sleep duration with 2min ago transport update: 180 seconds (~3 minutes)
```

### 3. `test_with_both_previous_updates()`

**Scenario:** Both weather and transport have previous updates

- Weather updated 1 hour ago (2-hour interval = 1 hour remaining)
- Transport updated 2 minutes ago (5-minute interval = 3 minutes remaining)
- **Expected:** Should sleep until nearest update (transport in ~3 minutes)

```cpp
uint32_t oneHourAgo = (uint32_t)now - (60 * 60);
TimingManager::setLastWeatherUpdate(oneHourAgo);

uint32_t twoMinutesAgo = (uint32_t)now - (2 * 60);
TimingManager::setLastTransportUpdate(twoMinutesAgo);
```

**Output:**

```
Sleep duration with both previous updates: 180 seconds (~3 minutes)
  Weather updated: 1 hour ago (interval: 2 hours)
  Transport updated: 2 minutes ago (interval: 5 minutes)
```

### 4. `test_weather_update_overdue()`

**Scenario:** Update is overdue (interval already passed)

- Weather updated 3 hours ago
- Interval is 2 hours (overdue by 1 hour!)
- **Expected:** Should wake up immediately (minimum 30 seconds)

```cpp
uint32_t threeHoursAgo = (uint32_t)now - (3 * 60 * 60);
TimingManager::setLastWeatherUpdate(threeHoursAgo);
```

**Output:**

```
Sleep duration when weather update is overdue: 30 seconds
```

### 5. `test_verify_timestamp_setters()`

**Scenario:** Verify setter/getter methods work correctly

- Sets timestamps using setters
- Retrieves using getters
- **Expected:** Values match what was set

```cpp
TimingManager::setLastWeatherUpdate(testTimestamp);
uint32_t retrieved = TimingManager::getLastWeatherUpdate();
TEST_ASSERT_EQUAL_UINT32(testTimestamp, retrieved);
```

**Output:**

```
Timestamp setters/getters verified successfully
```

## Test Results

```
11 Tests 0 Failures 0 Ignored
OK
```

All tests pass successfully! ✅

## How to Use in Your Tests

### Pattern 1: No Previous Update (First Run)

```cpp
TimingManager::setLastWeatherUpdate(0);
TimingManager::setLastTransportUpdate(0);
// Device will update immediately (minimum sleep)
```

### Pattern 2: Recent Update

```cpp
time_t now = time(nullptr);
uint32_t recentTime = (uint32_t)now - (5 * 60); // 5 minutes ago
TimingManager::setLastWeatherUpdate(recentTime);
// Device will wait for remaining interval time
```

### Pattern 3: Overdue Update

```cpp
time_t now = time(nullptr);
uint32_t oldTime = (uint32_t)now - (10 * 60 * 60); // 10 hours ago
TimingManager::setLastWeatherUpdate(oldTime);
// Device will update immediately (minimum sleep)
```

## Debugging Tips

1. **View timestamps during tests:**
   ```cpp
   printf("Current time: %u\n", (uint32_t)time(nullptr));
   printf("Last weather: %u\n", TimingManager::getLastWeatherUpdate());
   printf("Last transport: %u\n", TimingManager::getLastTransportUpdate());
   ```

2. **Set specific time scenarios:**
   ```cpp
   // Test midnight rollover
   // Test weekend transitions
   // Test active hours boundaries
   ```

3. **Use CLion debugger:**
    - Set breakpoints in test functions
    - Inspect `sleepDuration` variable
    - Step through `getNextSleepDurationSeconds()` logic

## Production Impact

✅ **No breaking changes** - Making the timestamp methods public only adds visibility for testing. Production code
continues to work exactly as before.

✅ **ESP32 build verified** - Production firmware still compiles and runs successfully.

