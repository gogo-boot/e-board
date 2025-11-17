# OTA Update Integration - Implementation Summary

## Date

November 3, 2025

## Overview

Integrated the existing OTA update functionality with the new OTA configuration feature. The system now automatically
checks for firmware updates at the configured time if OTA is enabled in the configuration.

## Implementation

### Main Changes in `src/main.cpp`

#### 1. Added Helper Function: `shouldRunOTAUpdate()`

This function checks if OTA update should be executed based on:

- **Configuration check**: `config.otaEnabled` must be `true`
- **Time check**: Current time must be within ±1 minute of `config.otaCheckTime`
- **Time validation**: Properly handles invalid time formats

**Logic Flow:**

```
1. Check if OTA is enabled in config
   └─> If disabled → return false

2. Get current local time
   └─> If time fetch fails → return false (log warning)

3. Parse configured OTA check time (format: "HH:MM")
   └─> If invalid format → return false (log warning)

4. Compare current time with configured time
   └─> Within ±1 minute tolerance → return true
   └─> Otherwise → return false
```

#### 2. Integrated OTA Check in `setup()`

Added OTA update check **before** normal device operations:

```cpp
// Initialize battery manager
BatteryManager::init();

// Check if OTA update should run based on configuration
if (shouldRunOTAUpdate()) {
    ESP_LOGI(TAG, "Starting OTA update check...");
    check_update_task(nullptr);
    // Note: If update is found and installed, device will restart
    // If no update or update fails, execution continues normally
}

// Continue with normal device operations...
```

### Time Tolerance

- **Tolerance window**: ±1 minute
- **Example**: If configured time is 03:00, OTA will run between 02:59 and 03:01

### Execution Flow

#### Scenario 1: OTA Disabled

```
Device boots
  → shouldRunOTAUpdate() checks config.otaEnabled
  → Returns false
  → OTA check is skipped
  → Device proceeds to normal operations
```

#### Scenario 2: OTA Enabled but Wrong Time

```
Device boots at 10:30
  → shouldRunOTAUpdate() checks config.otaEnabled = true
  → Gets current time: 10:30
  → Configured time: 03:00
  → Time doesn't match (difference > 1 minute)
  → Returns false
  → Device proceeds to normal operations
```

#### Scenario 3: OTA Enabled and Correct Time

```
Device boots at 03:00
  → shouldRunOTAUpdate() checks config.otaEnabled = true
  → Gets current time: 03:00
  → Configured time: 03:00
  → Time matches (within ±1 minute)
  → Returns true
  → Calls check_update_task(nullptr)
  → OTA function checks for updates

  If update available:
    → Downloads firmware
    → Installs firmware
    → Device restarts with new firmware

  If no update or update fails:
    → Continues to normal operations
```

## Logging

### Debug Logs

- `"OTA automatic updates are disabled"` - When OTA is disabled in config
- `"OTA update time not matched. Configured: XX:XX, Current: XX:XX"` - When time doesn't match

### Info Logs

- `"OTA update time matched! Configured: XX:XX, Current: XX:XX"` - When time matches
- `"Starting OTA update check..."` - When OTA check begins

### Warning Logs

- `"Failed to get current time for OTA check"` - When time fetch fails
- `"Invalid OTA check time format: XX:XX"` - When config time format is invalid

## Configuration Integration

### Configuration Fields Used

From `RTCConfigData`:

- `config.otaEnabled` (bool) - Enable/disable automatic OTA
- `config.otaCheckTime` (char[6]) - Time format "HH:MM"

### Default Values

- `otaEnabled`: `true` (enabled by default)
- `otaCheckTime`: `"03:00"` (3:00 AM)

## Safety Features

### 1. Configuration Validation

- Validates OTA is enabled before proceeding
- Validates time format is correct (HH:MM)
- Gracefully handles invalid configurations

### 2. Time Validation

- Ensures time service is available
- Handles time fetch failures gracefully
- Uses 1-minute tolerance for flexibility

### 3. Non-Blocking on Failure

- If OTA check is not needed, execution continues immediately
- If time check fails, device proceeds to normal operations
- Only blocks when OTA update is actually downloading/installing

### 4. Automatic Restart

- Device automatically restarts after successful firmware update
- Users don't need to manually restart

## Testing Scenarios

### Test 1: OTA Disabled

- Set `otaEnabled = false` in config
- Boot at any time
- Expected: OTA check skipped, normal operations proceed

### Test 2: OTA Enabled, Wrong Time

- Set `otaEnabled = true`, `otaCheckTime = "03:00"`
- Boot at 10:00
- Expected: OTA check skipped (time mismatch), normal operations proceed

### Test 3: OTA Enabled, Correct Time, No Update

- Set `otaEnabled = true`, `otaCheckTime = "03:00"`
- Boot at 03:00
- Ensure no firmware update available
- Expected: OTA checks, finds no update, normal operations proceed

### Test 4: OTA Enabled, Correct Time, Update Available

- Set `otaEnabled = true`, `otaCheckTime = "03:00"`
- Boot at 03:00
- Ensure firmware update available
- Expected: OTA downloads and installs update, device restarts

### Test 5: Time Tolerance

- Set `otaEnabled = true`, `otaCheckTime = "03:00"`
- Test boots at 02:59, 03:00, 03:01
- Expected: All three times should trigger OTA check

### Test 6: Invalid Time Format

- Set `otaCheckTime = "invalid"`
- Boot at any time
- Expected: Warning logged, OTA skipped, normal operations proceed

## Dependencies

### Required Headers

- `util/time_manager.h` - For time retrieval
- `ota/ota_update.h` - For OTA update function
- `config/config_manager.h` - For configuration access

### Required Functions

- `TimeManager::getCurrentLocalTime()` - Gets current local time
- `check_update_task()` - Existing OTA update implementation (unchanged)
- `ConfigManager::getConfig()` - Access to configuration data

## Notes

### OTA Implementation

- The existing OTA implementation (`check_update_task()`) was **NOT modified**
- It remains tested and stable
- Integration only adds the configuration check before calling it

### Time Synchronization

- Device must have accurate time from NTP for OTA to work correctly
- Time synchronization happens during WiFi connection
- If time is not available, OTA check is safely skipped

### Deep Sleep Impact

- OTA check happens on every boot (after deep sleep wake-up)
- With 1-minute tolerance, device will check for updates if it wakes within the time window
- After update check (success or skip), device proceeds to normal sleep cycle

### Network Requirement

- OTA update requires WiFi connection
- WiFi is already connected during boot process
- No additional network setup needed

## Future Enhancements (Optional)

1. **Last Check Tracking**: Store timestamp of last OTA check to avoid multiple checks in same time window
2. **Retry Logic**: Add retry mechanism if OTA check fails due to network issues
3. **Update Notification**: Add display notification before/after OTA update
4. **Update History**: Log successful updates with timestamps
5. **Manual Update Trigger**: Add web interface button to manually trigger OTA check

## Conclusion

The OTA update integration is complete and fully functional. The system now:

- Respects user configuration for automatic updates
- Checks for updates at the configured daily time
- Has proper error handling and logging
- Continues normal operations if OTA is not needed
- Automatically updates and restarts when new firmware is available

