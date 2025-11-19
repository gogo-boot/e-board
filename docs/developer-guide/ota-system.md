# OTA Firmware Update Configuration Feature

## Overview

This document describes the new OTA (Over-The-Air) firmware update configuration feature that allows users to
enable/disable automatic firmware updates and set a daily check time.

## Implementation Date

November 3, 2025

## Feature Description

The OTA configuration section allows users to:

- Enable or disable automatic firmware updates with a single click
- Set a specific time for daily firmware update checks
- See clear visual indicators of the OTA status

## User Interface

### Location

The OTA configuration section is positioned **above** the "Netzwerk-Information" section in the configuration page.

### Design

- **Section Title**: 🔄 Firmware-Aktualisierung (in German)
- **Toggle Buttons**: Two large, single-click buttons for better UX:
    - "Aktiviert" (Enabled) - with green status indicator
    - "Deaktiviert" (Disabled) - with red status indicator
- **Time Picker**: Allows selection of daily check time (default: 03:00 AM)
- **Help Text**: Explains that the device will check daily for updates
- **Warning**: Informs users that the device will be unavailable for 1-2 minutes during update

### CSS Styling

The design follows the existing configuration page style:

- Blue accent color (#007aff)
- Rounded borders and shadows
- Responsive design for mobile devices
- Hover effects for better interactivity

## Backend Implementation

### Files Modified

1. **data/config_my_station.html**
    - Added HTML structure for OTA configuration section
    - Added CSS styles for toggle buttons
    - Added JavaScript function `toggleOTA()` to handle button clicks
    - Added OTA configuration to save handler
    - Added initialization code to set toggle state on page load

2. **include/config/config_struct.h**
    - Added `otaEnabled` (bool) field
    - Added `otaCheckTime` (String) field

3. **include/config/config_manager.h**
    - Added `otaEnabled` (bool) field to RTCConfigData struct
    - Added `otaCheckTime` (char[6]) field to RTCConfigData struct
    - Updated total memory usage comment

4. **src/config/config_manager.cpp**
    - Added default values for OTA configuration in RTC memory initialization
    - Added OTA configuration loading from NVS
    - Added OTA configuration saving to NVS

5. **src/config/config_page.cpp**
    - Added OTA configuration placeholder replacement in `handleConfigPage()`
    - Added OTA configuration parsing in `handleSaveConfig()`

## Data Structure

### RTCConfigData (RTC Memory)

```cpp
bool otaEnabled;        // 1 byte - Enable/disable automatic OTA updates
char otaCheckTime[6];   // 6 bytes - Time format "HH:MM"
```

### NVS Storage Keys

- `otaEnabled`: Boolean value
- `otaCheckTime`: String value (format: "HH:MM")

## Default Values

- **otaEnabled**: `true` (enabled by default)
- **otaCheckTime**: `"03:00"` (3:00 AM)

## Configuration Flow

1. User opens configuration page
2. OTA section displays current settings (loaded from NVS/RTC memory)
3. User clicks "Aktiviert" or "Deaktiviert" button (single click)
4. If enabled, user can optionally adjust the check time
5. User clicks "Speichern" (Save) button
6. Configuration is sent to backend via POST request
7. Backend saves to both RTC memory and NVS
8. Device restarts with new configuration

## JavaScript Functions

### toggleOTA(enabled)

- **Purpose**: Toggle the OTA configuration buttons and show/hide time picker
- **Parameters**:
    - `enabled` (boolean): true to enable OTA, false to disable
- **Behavior**:
    - Updates button active states
    - Shows/hides the time picker container
    - Called on page load and button clicks

## Integration Points

### Future OTA Implementation

The saved configuration can be accessed via:

```cpp
RTCConfigData& config = ConfigManager::getConfig();
bool shouldCheckOTA = config.otaEnabled;
const char* checkTime = config.otaCheckTime;
```

### Suggested Implementation

1. Check if OTA is enabled: `config.otaEnabled`
2. Parse check time: `config.otaCheckTime` (format: "HH:MM")
3. Schedule daily check at specified time
4. Download and install firmware if available
5. Restart device after successful update

## UX Improvements

- **Single-click toggle**: Users don't need to open dropdown menus
- **Visual indicators**: Green/red dots show enabled/disabled status clearly
- **Contextual time picker**: Only visible when OTA is enabled
- **German language**: All text is in German as requested
- **Warning message**: Users are informed about temporary unavailability

## Memory Impact

- Added 7 bytes to RTCConfigData struct (1 byte bool + 6 bytes char array)
- Total RTCConfigData size: ~529 bytes (still well under 8KB RTC limit)

## Testing Checklist

- [ ] Configuration page loads with OTA section
- [ ] Toggle buttons work with single click
- [ ] Time picker is shown/hidden correctly
- [ ] Save functionality includes OTA configuration
- [ ] Configuration persists after device restart
- [ ] Configuration loads correctly from NVS
- [ ] Default values are set correctly for new devices

## Notes

- The actual OTA update mechanism is not implemented in this feature
- This feature only provides the UI and configuration storage
- Future work should implement the actual firmware update logic based on these settings

