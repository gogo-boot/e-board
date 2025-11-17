# OTA Update Integration with Sleep Timing - Implementation Summary

## Date

November 3, 2025

## Overview

Successfully integrated OTA update scheduling into the `TimingManager` to ensure firmware updates can occur even during
deep sleep periods. The device now intelligently schedules wake-ups for OTA checks alongside weather and transport
updates.

## Architecture

### Key Components

1. **TimingManager** (`util/timing_manager.cpp`)
    - Calculates optimal sleep duration considering weather, transport, and OTA updates
    - Manages RTC memory for update timestamps
    - Bypasses sleep restrictions for OTA updates

2. **Main.cpp** (`src/main.cpp`)
    - Checks if OTA should run at boot time
    - Executes OTA check if conditions are met
    - Marks OTA check timestamp after execution

3. **Configuration** (`config/config_manager.h`)
    - `otaEnabled`: Boolean flag for automatic updates
    - `otaCheckTime`: Daily check time (format: "HH:MM")

## Implementation Details

### New RTC Memory Variable

```cpp
RTC_DATA_ATTR uint32_t lastOTACheck = 0;
```

- Survives deep sleep
- Tracks last OTA check timestamp
- Prevents duplicate checks within 2-minute window

### Helper Function: `calculateNextOTACheckTime()`

**Purpose**: Calculate when the next OTA check should occur

**Logic**:

1. Check if OTA is enabled → return 0 if disabled
2. Check if OTA was checked recently (< 2 minutes) → return 0 to skip
3. Parse configured OTA time (e.g., "03:00")
4. Compare with current time
5. Calculate next occurrence (today or tomorrow)
6. Return timestamp in seconds

**Features**:

- ✅ Respects `otaEnabled` configuration
- ✅ Prevents repeated checks (2-minute cooldown)
- ✅ Handles day transitions (today vs tomorrow)
- ✅ Returns 0 if OTA shouldn't be scheduled

### Modified: `getNextSleepDurationSeconds()`

**Changes Made**:

1. **Added OTA calculation**:
   ```cpp
   uint32_t nextOTACheckSeconds = calculateNextOTACheckTime(currentTimeSeconds);
   ```

2. **Updated nearest update logic**:
    - Now considers 3 update types: weather, transport, and OTA
    - Finds minimum of all scheduled updates
    - Tracks if nearest update is OTA with `isOTAUpdate` flag

3. **Modified sleep period handling**:
   ```cpp
   if (isUpdateInSleepPeriod) {
       if (isOTAUpdate) {
           // OTA bypasses sleep period - keep wake time
       } else {
           // Normal behavior: skip to sleep end
       }
   }
   ```

### Key Behaviors

#### 1. OTA Disabled

- No OTA calculations performed
- Sleep duration determined by weather/transport only
- Zero overhead when feature is disabled

#### 2. OTA Enabled, Before Other Updates

- Device wakes specifically for OTA check
- Sleeps until OTA time even if during transport inactive hours
- Example: Wake at 3:00 AM for OTA, skip transport until 6:00 AM

#### 3. OTA During Deep Sleep

- **Critical Feature**: OTA bypasses sleep period
- Device wakes for OTA even at 3:00 AM during sleep (22:30 - 05:30)
- After OTA, normal sleep schedule resumes

#### 4. OTA Already Checked

- 2-minute cooldown prevents repeated checks
- If checked at 3:00, won't check again until 3:02
- Allows normal sleep calculations to proceed

#### 5. Multiple Updates Coincide

- System wakes once for earliest update
- Example: If weather and OTA both at 3:00 AM, wake once
- All pending updates processed at that time

## Flow Diagrams

### Sleep Calculation with OTA

```
┌─────────────────────────────────────────┐
│   Calculate Next Update Times           │
├─────────────────────────────────────────┤
│  1. Weather update (if needed)          │
│  2. Transport update (if needed)        │
│  3. OTA check (if enabled)              │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│   Find Nearest Update Time              │
│   min(weather, transport, OTA)          │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│   Is Nearest Update = OTA?              │
└──────────────┬──────────────────────────┘
               │
        ┌──────┴──────┐
        │             │
       YES           NO
        │             │
        ▼             ▼
┌─────────────┐  ┌──────────────┐
│ Check Sleep │  │ Apply Normal │
│   Period    │  │ Sleep Logic  │
└──────┬──────┘  └──────┬───────┘
       │                │
       ▼                ▼
  In Sleep?        In Sleep?
   │    │            │    │
  YES  NO          YES  NO
   │    │            │    │
   │    └────┬───────┘    │
   │         │            │
   └─────────┼────────────┘
             ▼
     ┌───────────────┐
     │ Calculate     │
     │ Sleep Duration│
     └───────────────┘
```

### OTA Priority Example

**Scenario**: Device at 11:00 PM

- Sleep period: 22:30 - 05:30
- OTA time: 03:00
- Weather interval: 3 hours
- Transport inactive (06:00 - 09:00)

**Without OTA Integration**:

```
23:00 → Sleep until 05:30 (6.5 hours)
05:30 → Wake, skip transport (inactive)
05:30 → Weather update
06:00 → Resume normal operation
```

**With OTA Integration**:

```
23:00 → Sleep until 03:00 (4 hours)
03:00 → Wake for OTA check ✅
03:00 → Check firmware, update if available
03:00 → Sleep until 05:30 (2.5 hours)
05:30 → Wake for normal operations
```

## Test Coverage

### Unit Tests Added (10 tests)

1. **test_ota_disabled**
    - Verifies OTA doesn't affect sleep when disabled
    - Expects normal weather/transport calculations

2. **test_ota_enabled_before_other_updates**
    - OTA is nearest update
    - Device wakes specifically for OTA

3. **test_ota_during_deep_sleep_bypasses_sleep**
    - **Most Important**: Proves OTA bypasses sleep restrictions
    - Wakes at 3:00 AM instead of 5:30 AM

4. **test_ota_already_checked_recently**
    - Verifies 2-minute cooldown works
    - Prevents repeated OTA checks

5. **test_ota_scheduled_later_today**
    - OTA time hasn't occurred yet today
    - Calculates correct sleep duration

6. **test_ota_scheduled_tomorrow**
    - Current time past today's OTA time
    - OTA scheduled for next day

7. **test_ota_and_weather_coincide**
    - Both updates at same time
    - Device wakes once for both

8. **test_ota_during_transport_inactive_hours**
    - OTA runs even when transport is inactive
    - Doesn't wait for transport active hours

9. **test_ota_on_weekend**
    - Weekend mode doesn't affect OTA
    - Uses weekend sleep settings but still wakes for OTA

10. **test_ota_timestamp_management**
    - Verifies getter/setter functions
    - Confirms RTC memory persistence

### Running Tests

```bash
cd /Users/jinwoowang/project/gogo-boot/mystation
pio test -e native -f test_sleep_duration
```

## Configuration Examples

### Example 1: Daily 3 AM Check (Default)

```cpp
config.otaEnabled = true;
strcpy(config.otaCheckTime, "03:00");
```

**Behavior**: Device wakes every day at 3:00 AM to check for firmware updates, regardless of sleep schedule.

### Example 2: Disabled Updates

```cpp
config.otaEnabled = false;
```

**Behavior**: No OTA checks, zero performance impact, normal sleep behavior.

### Example 3: Early Morning Check

```cpp
config.otaEnabled = true;
strcpy(config.otaCheckTime, "05:00");
```

**Behavior**: Wakes at 5:00 AM (near sleep end at 5:30 AM), efficient timing.

## Performance Considerations

### Memory Impact

- **Added**: 4 bytes RTC memory (`lastOTACheck`)
- **Total RTC Memory**: Still well under 8KB limit
- **Runtime**: Minimal - only calculates when needed

### Power Consumption

- OTA check adds one wake event per day
- Wake duration: 1-2 minutes (including download/install if available)
- Negligible impact on overall battery life

### Network Usage

- One HTTPS request per day
- Downloads ~200 bytes JSON for version check
- Downloads firmware only if update available

## Edge Cases Handled

### 1. Time Not Available

- If NTP time not synced, OTA check skipped gracefully
- Logged as warning, doesn't crash

### 2. OTA Time During Transport Active Hours

- OTA takes priority
- Transport updates continue after OTA

### 3. Multiple Wake Events Same Minute

- 2-minute cooldown prevents duplicate checks
- Efficient wake-once strategy

### 4. Weekend/Weekday Transitions

- OTA scheduled based on actual time, not day type
- Works across Friday→Saturday and Sunday→Monday

### 5. Day Transitions

- Correctly calculates "tomorrow" OTA time
- Handles midnight rollover properly

## Future Enhancements (Optional)

### 1. Variable Check Intervals

- Currently: Daily at fixed time
- Future: Every N days (e.g., weekly, bi-weekly)

### 2. Update History

- Track successful updates
- Store last update version and timestamp

### 3. Retry Logic

- If check fails (network issues), retry after X minutes
- Max retries per day

### 4. User Notification

- Display "Update available" message
- Show update progress bar

### 5. Multiple Check Times

- Support multiple daily check windows
- Example: 3:00 AM and 3:00 PM

## Logging

### Debug Logs

```
[DEBUG] OTA automatic updates are disabled
[DEBUG] OTA check already performed recently (within 2 minutes)
[DEBUG] Next OTA check is later today in 45 minutes at 03:00
```

### Info Logs

```
[INFO] Next OTA check scheduled at: 1730620800 seconds
[INFO] OTA update scheduled during sleep period - bypassing sleep restrictions
[INFO] All updates needed - nearest at: 1730620800 seconds
```

### Warning Logs

```
[WARN] Failed to get current time for OTA check
[WARN] Invalid OTA check time format: XX:XX
```

## API Reference

### Public Functions

#### `TimingManager::getLastOTACheck()`

Returns last OTA check timestamp (seconds since epoch)

#### `TimingManager::setLastOTACheck(uint32_t timestamp)`

Sets last OTA check timestamp (for testing and after actual check)

### Private Functions

#### `TimingManager::calculateNextOTACheckTime(uint32_t currentTimeSeconds)`

Calculates next OTA check time considering:

- OTA enabled status
- Last check time (cooldown)
- Configured check time
- Current time of day

Returns 0 if OTA shouldn't be scheduled, otherwise timestamp in seconds.

## Integration Checklist

- [x] Add RTC memory variable for OTA timestamp
- [x] Implement calculateNextOTACheckTime()
- [x] Integrate OTA into getNextSleepDurationSeconds()
- [x] Add sleep bypass logic for OTA
- [x] Update main.cpp to mark OTA timestamp
- [x] Add comprehensive unit tests
- [x] Update configuration handling
- [x] Document implementation
- [x] Test all edge cases

## Success Criteria

✅ **OTA updates can occur during deep sleep**

- Device wakes at configured time even during sleep period

✅ **No duplicate checks**

- 2-minute cooldown prevents repeated checks

✅ **Zero impact when disabled**

- Disabled OTA has no performance overhead

✅ **Proper priority handling**

- OTA, weather, and transport updates coexist peacefully

✅ **Comprehensive test coverage**

- 10 unit tests covering all scenarios

✅ **Production ready**

- Proper error handling, logging, and edge case management

## Conclusion

The OTA update integration with sleep timing is complete and thoroughly tested. The system now intelligently schedules
firmware update checks at the configured daily time, bypassing sleep restrictions when necessary while maintaining
efficient power management for normal operations.

**Key Achievement**: Device can now receive critical firmware updates even during overnight deep sleep periods, ensuring
security patches and bug fixes are applied promptly without user intervention.

