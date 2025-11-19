# Quick Start Guide

Get your MyStation E-Board up and running in 5 minutes!

## Prerequisites Checklist

- [ ] ESP32-C3 Super Mini with e-paper display connected
- [ ] PlatformIO development environment setup
- [ ] API keys configured (Google, RMV)
- [ ] Firmware compiled and uploaded

## Understanding Device Boot Process

### First Boot (Fresh Device)

When you power on the device for the first time:

- Device automatically detects it has no saved configuration
- Enters **Configuration Mode** and creates WiFi hotspot
- Waits for user to complete initial setup
- All settings are permanently saved to flash memory

### Subsequent Boots

After initial configuration:

- Device loads saved settings from flash memory
- Connects directly to your WiFi network
- Starts normal operation (fetch data → display → sleep)
- No manual intervention needed

### Power Loss Recovery

If battery dies or power is disconnected:

- Configuration remains safely stored in flash memory
- Device automatically resumes normal operation when powered on
- No need to reconfigure WiFi or preferences

## Step 1: First Boot

### Power On

1. Connect ESP32-C3 to power (USB-C or battery)
2. Open serial monitor: `pio device monitor`
3. Look for startup messages:
   ```
   [MAIN] System starting...
   [MAIN] Fresh boot: Loaded config mode from NVS: true
   [MAIN] WiFiManager AP mode started
   [MAIN] AP SSID: MyStation-ABCD1234
   ```

### Initial Configuration Mode

The device starts in **configuration mode** on first boot and will:

- Create a WiFi access point for setup
- Wait for user configuration via web interface
- Save all settings permanently to flash memory
- Switch to operational mode after configuration

## Step 2: WiFi Setup

### Connect to Configuration AP

1. **Find WiFi network**: `MyStation-XXXXXXXX` (unique ID)
2. **Connect** with any device (phone, laptop)
3. **Open browser** to `http://10.0.1.1`
4. **Configure WiFi**:
    - Select your home WiFi network
    - Enter WiFi password
    - Click "Save"

### Connection Success

Device will:

- Connect to your WiFi
- Get IP address via DHCP
- Start mDNS responder
- Begin location detection

## Step 3: Location & Transport Setup

### Automatic Location Detection

The device will automatically:

1. **Detect location** using Google Geolocation API
2. **Find nearby stops** using RMV transport API
3. **Display found stops** in configuration interface

### Access Configuration Interface

After WiFi setup, access via:

- **Local IP**: `http://192.168.1.XXX` (check serial monitor)
- **mDNS**: `http://mystation.local` (if supported)

## Step 4: Configure Your Station

### Web Interface

The configuration page allows you to:

#### 1. Location Settings

- **Current Location**: Automatically detected
- **City**: Auto-populated from coordinates
- **Manual Override**: Enter custom coordinates if needed

#### 2. Transport Selection

- **Available Stops**: List of nearby public transport stops
- **Select Primary Stop**: Choose your main departure station
- **Transport Filters**: Select transport types (RE, S-Bahn, Bus, etc.)

#### 3. Display Options

- **Update Interval**: How often to refresh data (default: 5 minutes)
- **Sleep Schedule**: Optional time-based sleep (e.g., 23:00 - 06:00)
- **Weather Display**: Enable/disable weather information

#### 4. Privacy Settings

- **Data Storage**: All data stays on device
- **API Usage**: Only for data retrieval, no tracking
- **Local Processing**: No data sent to third parties

### Save Configuration

1. **Review settings** in the web interface
2. **Click "Save Configuration"**
3. **Wait for confirmation** message
4. **Device will restart** in operational mode

## Step 5: Configuration Complete & Transition

### What Happens After Saving

When you save the configuration:

1. **Settings Stored Permanently**: All preferences saved to flash memory (NVS)
    - WiFi credentials
    - Selected transport stop and filters
    - Update intervals and display preferences
    - Location coordinates and city name

2. **Device Restarts Automatically**: Switches from configuration mode to operational mode

3. **Normal Operation Begins**: Device will now:
    - Connect directly to your WiFi
    - Start fetching real-time data
    - Update the display
    - Enter power-saving deep sleep between updates

### Future Boots

From now on, every time the device powers on:

- **Loads saved configuration** from permanent storage
- **Connects to WiFi automatically** (no hotspot created)
- **Starts normal operation immediately** (no configuration needed)
- **Runs indefinitely** with your saved preferences

> **💡 Important**: Configuration persists across power loss, battery changes, and firmware updates. The device only
> enters configuration mode again if explicitly reset or if configuration is corrupted.

## Step 6: Normal Operation

### Operational Mode

In normal operation, the device:

1. **Wakes up** at configured intervals
2. **Fetches data**: Transport departures + weather
3. **Updates display**: Shows current information
4. **Enters deep sleep**: Conserves battery power

### Expected Behavior

#### On Wake:

```
[MAIN] Wakeup caused by timer
[MAIN] Current time: 2025-06-30 14:25:00
[RMV] Using stop: Frankfurt Hauptbahnhof
[RMV] Found 8 departures in next 60 minutes
[DWD] Weather: 22°C, partly cloudy
[MAIN] Sleeping for 300 seconds until next interval
```

#### Display Updates:

- **Departure times**: Next 3-5 departures
- **Weather info**: Current temperature and conditions
- **Status indicators**: WiFi connection, battery level
- **Last updated**: Timestamp of data refresh

## Step 7: Verification

### Check Operation

Monitor for 15-30 minutes to verify:

- [ ] Regular wake/sleep cycles (every 5 minutes)
- [ ] Successful data fetching
- [ ] Display updates working
- [ ] Deep sleep power consumption low

### Serial Monitor Output

Healthy operation shows:

```
[SLEEP] Wakeup caused by timer
[MAIN] Current time: 14:30:00
[RMV] Departures updated successfully
[DWD] Weather updated successfully
[SLEEP] Entering deep sleep for 300 seconds
```

## Troubleshooting Quick Fixes

### Device Won't Start

- **Check power supply** (3.3V, sufficient current)
- **Verify serial connection** (115200 baud)
- **Try factory reset**: Hold user button during power-on

### WiFi Connection Issues

- **Reset WiFi settings**: Uncomment `wm.resetSettings()` in code
- **Check signal strength**: Move closer to router
- **Verify credentials**: Re-enter WiFi password

### No Transport Data

- **Check location**: Verify you're in RMV coverage area (Hesse, Germany)
- **Verify API keys**: Check RMV API key validity
- **Test with known stop**: Try Frankfurt Hauptbahnhof (ID: 3006907)

### Configuration Interface Not Loading

- **Check IP address**: Look in serial monitor for correct IP
- **Try mDNS**: `http://mystation.local`
- **Clear browser cache**: Force refresh (Ctrl+F5)

### Display Not Updating

- **Check e-paper connections**: Verify all 8 pins connected
- **Monitor SPI signals**: Use oscilloscope if available
- **Test with simple display**: Draw basic shapes/text

## Next Steps

### After Basic Setup

- [Configuration Guide](developer-guide/configuration-system.md) - Advanced options
- [Troubleshooting](user-guide/troubleshooting.md) - Detailed problem solving
- [User Guide](user-guide/quick-start-simplified.md) - Simplified setup guide

### Customization

- [API Integration](developer-guide/api-integration.md) - External APIs
- [Development Guide](developer-guide/development-setup.md) - Extend functionality
- [OTA System](developer-guide/ota-system.md) - Remote firmware updates

## Support

### Getting Help

- **Serial monitor**: Most issues show up in debug output
- **Documentation**: Check specific guides for detailed help
- **Community**: Share experiences and solutions

### Common Success Indicators

✅ **WiFi connected and stable**
✅ **Location detected correctly**
✅ **Transport stops found**
✅ **Regular data updates**
✅ **Deep sleep working**
✅ **Display showing current information**

Congratulations! Your MyStation E-Board is now operational. 🎉
