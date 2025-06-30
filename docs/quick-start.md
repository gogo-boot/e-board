# Quick Start Guide

Get your MyStation E-Board up and running in 5 minutes!

## Prerequisites Checklist
- [ ] ESP32-C3 Super Mini with e-paper display connected
- [ ] PlatformIO development environment setup
- [ ] API keys configured (Google, RMV)
- [ ] Firmware compiled and uploaded

## Step 1: First Boot

### Power On
1. Connect ESP32-C3 to power (USB-C or battery)
2. Open serial monitor: `pio device monitor`
3. Look for startup messages:
   ```
   [MAIN] System starting...
   [MAIN] WiFiManager AP mode started
   [MAIN] AP SSID: MyStation-ABCD1234
   ```

### Initial Configuration Mode
The device starts in **configuration mode** on first boot.

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

## Step 5: Normal Operation

### Operational Mode
After configuration, the device:
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

## Step 6: Verification

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
- [Configuration Guide](./configuration.md) - Advanced options
- [Deep Sleep Configuration](./deep-sleep.md) - Power optimization
- [Troubleshooting](./troubleshooting.md) - Detailed problem solving

### Customization
- [API Reference](./api-reference.md) - Modify web interface
- [Development Guide](./development.md) - Extend functionality
- [OTA Updates](./ota-updates.md) - Remote firmware updates

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
