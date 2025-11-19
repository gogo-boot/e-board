# Troubleshooting Guide

This guide helps you solve common issues with your MyStation e-paper departure board.

## Quick Diagnostics

### Check Serial Monitor First

Most problems can be diagnosed by checking the serial monitor:

```bash
pio device monitor
```

Look for error messages, warnings, or unexpected behavior in the output.

---

## WiFi & Network Issues

### Cannot Connect to MyStation WiFi Access Point

**Symptoms:**

- `MyStation-XXXXXXXX` network not visible
- Can't join configuration WiFi

**Solutions:**

1. **Wait for startup** (30-60 seconds after power on)
    - Device needs time to boot
    - Check serial monitor for `AP mode started`

2. **Check WiFi band**
    - ⚠️ MyStation only works with **2.4 GHz WiFi**
    - If your phone is set to "5 GHz only", change to "Auto" or "2.4 GHz"
    - Some phones hide 2.4 GHz networks by default

3. **Verify device is in config mode**
    - Serial monitor should show: `[MAIN] WiFiManager AP mode started`
    - If not, try [Factory Reset](factory-reset.md)

4. **Power cycle device**
    - Disconnect USB and battery
    - Wait 10 seconds
    - Reconnect power

5. **Try different device**
    - Use different phone/computer
    - Some devices have WiFi compatibility issues

### Cannot Connect MyStation to Home WiFi

**Symptoms:**

- Configuration page shows "Failed to connect to WiFi"
- Device keeps restarting
- Can't complete WiFi setup

**Solutions:**

1. **Verify WiFi network is 2.4 GHz**
    - ⚠️ **Critical**: MyStation only supports 2.4 GHz networks
    - 5 GHz networks are **not supported** (high energy consumption)
    - Check your router settings
    - Many routers have separate 2.4 GHz and 5 GHz SSIDs
    - Look for network names like "MyNetwork-2.4G" or "MyNetwork"

2. **Check WiFi password**
    - Verify password is correct (case-sensitive)
    - Watch for special characters
    - Try typing password in notes app first, then copy/paste

3. **Check WiFi signal strength**
    - Move device closer to router
    - Ensure no thick walls or metal objects blocking signal
    - Signal strength should be at least -70 dBm

4. **Router compatibility**
    - ESP32 supports: WPA/WPA2 Personal
    - NOT supported: WPA3, WPA2 Enterprise
    - Check router security settings

5. **Disable WiFi features**
    - Turn off AP Isolation
    - Disable Client Isolation
    - Enable DHCP
    - Allow new device connections

6. **Check MAC address filtering**
    - If enabled on router, add MyStation's MAC address
    - MAC address shown in serial monitor during boot

### Device Connects but Can't Access Internet

**Symptoms:**

- WiFi shows connected
- Display doesn't update
- "Failed to fetch data" errors

**Solutions:**

1. **Check internet connection**
    - Verify router has internet access
    - Test with other devices

2. **Check firewall settings**
    - Router firewall might block HTTPS requests
    - Whitelist MyStation's IP
    - Allow outbound connections on ports 80, 443

3. **DNS issues**
    - Router might have DNS problems
    - Check serial monitor for DNS resolution errors

4. **Verify API keys configured**
    - Google Geolocation API
    - RMV API key
    - See [Configuration](station-configuration.md)

### mDNS Not Working (Can't Access mystation.local)

**Symptoms:**

- `http://mystation.local` doesn't work
- "Server not found" error

**Solutions:**

1. **Use IP address instead**
    - Check serial monitor for device IP
    - Use `http://192.168.1.XXX` directly

2. **Platform compatibility**
    - ✅ Works on: macOS, Linux, iOS
    - ⚠️ Windows: Requires Bonjour service installed
    - ⚠️ Android: Limited mDNS support

3. **Network configuration**
    - mDNS requires multicast support
    - Some routers block mDNS
    - Try on different network

---

## Display Issues

### Display Not Updating

**Symptoms:**

- E-paper shows old data
- Display never refreshes
- Stuck on initial screen

**Solutions:**

1. **Check WiFi connection**
    - Verify device connects to 2.4 GHz WiFi
    - Check serial monitor: `[WIFI] Connected to: YourNetwork`

2. **Verify update interval**
    - Default: 5 minutes
    - Device sleeps between updates
    - Wait for next scheduled update

3. **Check API errors**
    - Serial monitor shows API failures
    - Verify API keys are configured
    - Check API service status

4. **Power cycle device**
    - Full restart can fix stuck states
    - Disconnect power for 10 seconds

5. **Check deep sleep issues**
    - Serial monitor shows sleep/wake cycles
    - If stuck in sleep, battery might be too low

### Display Shows Corrupted/Garbled Image

**Symptoms:**

- Random pixels or noise
- Partial image corruption
- Wrong colors (on BW display)

**Solutions:**

1. **Check wiring connections**
    - Verify all 8 pins connected correctly
    - Check for loose wires
    - Re-solder suspect joints

2. **Verify pin mapping**
    - ESP32-C3 vs ESP32-S3 use different GPIOs
    - Check `pins.h` matches your board
    - Rebuild firmware for correct board type

3. **SPI communication issues**
    - SCK, MOSI pins most critical
    - Check continuity with multimeter
    - Look for electrical noise/interference

4. **Power supply problems**
    - Display needs stable 3.3V
    - Low voltage can cause corruption
    - Try with USB power (not battery)

5. **Display hardware fault**
    - Try simple test pattern
    - If always corrupted, display may be damaged

### Display Shows Nothing (Blank)

**Symptoms:**

- Display completely blank/white
- No visible content
- Display never initializes

**Solutions:**

1. **Check display power**
    - Verify 3.3V and GND connected
    - Measure voltage at display connector
    - Should be 3.2-3.4V

2. **Check control signals**
    - CS (Chip Select)
    - DC (Data/Command)
    - RES (Reset)
    - BUSY
    - All must be connected

3. **Verify display compatibility**
    - Code configured for GDEY075T7
    - Check display model matches

4. **Check initialization**
    - Serial monitor shows display init steps
    - Look for errors during startup

5. **Test with simple code**
    - Upload basic GxEPD2 example
    - Verifies display hardware works

### Display Updates Too Slowly

**Symptoms:**

- Takes several minutes to update
- Device seems slow

**This is normal!**

- Full e-paper refresh: 30-45 seconds
- WiFi connection: 5-10 seconds
- Data fetching: 5-15 seconds
- Total cycle: 40-70 seconds is expected

**To improve:**

- Reduce data complexity
- Check WiFi signal strength
- Verify internet speed adequate

### Display Has "Ghost" Images

**Symptoms:**

- Previous image still faintly visible
- Overlapping images

**Solutions:**

1. **This is normal for e-paper**
    - E-paper displays can show ghosting
    - Periodic full refresh reduces this

2. **Enable/verify full refresh**
    - Code should do full refresh periodically
    - Full refresh clears ghosts

3. **Display wearing out**
    - E-paper has limited refresh cycles
    - Ghosting increases with age
    - ~10,000-100,000 full refreshes typical

---

## Power & Battery Issues

### Battery Drains Too Quickly

**Symptoms:**

- Battery lasts less than expected
- Need to charge daily

**Expected Battery Life:**

- ESP32-C3: 1-4 weeks (1000-2500mAh, 5-min updates)
- ESP32-S3: Similar performance

**Solutions:**

1. **Increase update interval**
    - Change from 5 to 10 or 15 minutes
    - Doubles/triples battery life
    - Still provides current information

2. **Enable sleep schedule**
    - Configure quiet hours (e.g., 23:00-06:00)
    - Save power overnight
    - See [Station Configuration](station-configuration.md)

3. **Check WiFi strength**
    - Weak signal uses more power
    - Move closer to router
    - Consider WiFi range extender

4. **Reduce display complexity**
    - Weather-only mode uses less data
    - Fewer API calls = less power

5. **Check for wake loops**
    - Serial monitor shows unexpected wakes
    - Device should sleep between updates
    - May indicate software issue

6. **Battery health**
    - Old LiPo batteries lose capacity
    - Check battery voltage (should be 3.7-4.2V)
    - Replace if battery damaged/old

### Device Won't Power On

**Symptoms:**

- No LED when plugged in
- Completely dead
- Won't charge

**Solutions:**

1. **Try different USB cable**
    - Some cables are charge-only (no data)
    - Try known-good cable

2. **Try different power source**
    - Different USB port
    - Different charger
    - Computer USB vs wall adapter

3. **Check battery polarity**
    - ⚠️ Wrong polarity can damage board
    - Red to red, black to black
    - Verify with multimeter

4. **Disconnect battery**
    - Test with USB power only
    - Battery fault might prevent boot

5. **Check for short circuits**
    - Visual inspection of solder joints
    - Look for burn marks
    - Smell for burning electronics

6. **Hardware failure**
    - Board may be damaged
    - Try known-good ESP32 board

### Battery Charging Issues (ESP32-S3)

**Symptoms:**

- Battery won't charge
- Charging very slow
- Battery gets hot

**Solutions:**

1. **Check charging circuit**
    - Some boards have built-in charger
    - Some require external charger
    - Verify your board's capabilities

2. **Use appropriate charger**
    - LiPo charging: 4.2V max
    - Current: 0.5C to 1C (500mA-1000mA for 1000mAh)
    - Don't exceed battery specs

3. **Monitor temperature**
    - Battery shouldn't get hot during charge
    - Warm is OK, hot indicates problem
    - Stop charging if too hot

4. **Battery protection circuit**
    - Most LiPo have built-in protection
    - May prevent charge if over-discharged
    - Try different battery

### Battery Voltage Showing Wrong (ESP32-S3)

**Symptoms:**

- Battery percentage seems incorrect
- Voltage reading doesn't match multimeter

**Solutions:**

1. **Calibration needed**
    - ADC readings need calibration
    - Voltage divider ratio might be off
    - Check developer documentation

2. **Check ADC enable pin**
    - GPIO 6 should enable voltage measurement
    - Verify in code

3. **Measure directly**
    - Use multimeter on battery terminals
    - Compare with displayed value
    - Calculate correction factor

---

## Button Issues (ESP32-S3 Only)

### Buttons Don't Respond

**Symptoms:**

- Pressing buttons does nothing
- Display mode doesn't change

**Solutions:**

1. **Verify button wiring**
    - Button 1: GPIO 2 to GND
    - Button 2: GPIO 3 to GND
    - Button 3: GPIO 5 to GND

2. **Check button type**
    - Must be momentary switches (normally open)
    - Press to close circuit
    - Release to open circuit

3. **Test button continuity**
    - Use multimeter in continuity mode
    - Should beep when button pressed
    - Should not beep when released

4. **Check firmware build**
    - Button support is ESP32-S3 only
    - Verify built for correct board
    - `BOARD_ESP32_S3` should be defined

5. **Enable button wakeup**
    - Check code enables GPIO wakeup
    - Buttons monitored during sleep

### Wrong Display Mode When Button Pressed

**Symptoms:**

- Button 1 shows wrong mode
- Buttons seem swapped

**Solutions:**

1. **Verify GPIO assignment**
    - Check which button goes to which GPIO
    - May be wired differently than expected

2. **Check pin definitions**
    - `pins.h` defines button mapping
    - Verify matches your wiring

3. **Rebuild firmware**
    - Ensure latest code deployed
    - Check no build errors

### Factory Reset Button Not Working

**Symptoms:**

- Hold Button 1 for 5 seconds, nothing happens
- No factory reset occurs

**Solutions:**

1. **Timing is critical**
    - Must hold DURING power-on
    - Continue holding for full 5 seconds
    - Count slowly: 1-Mississippi, 2-Mississippi...

2. **Check serial monitor**
    - Should show countdown messages
    - If nothing, button not detected

3. **Verify button 1 wiring**
    - GPIO 2 to GND when pressed
    - Check continuity

4. **Try other reset methods**
    - See [Factory Reset](factory-reset.md)
    - Manual NVS erase alternative

---

## Configuration Issues

### Can't Access Configuration Page

**Symptoms:**

- Can't load `http://10.0.1.1` or `http://mystation.local`
- Page won't load
- Connection timeout

**Solutions:**

1. **Verify connected to correct WiFi**
    - In config mode: Connect to `MyStation-XXXXXXXX`
    - In normal mode: Same network as MyStation

2. **Check device mode**
    - Config mode: Use `http://10.0.1.1`
    - Normal mode: Use device IP or `http://mystation.local`

3. **Clear browser cache**
    - Hard refresh: Ctrl+Shift+R (PC) or Cmd+Shift+R (Mac)
    - Try different browser
    - Try incognito/private mode

4. **Check SPIFFS uploaded**
    - Web files must be uploaded to device
    - Run: `pio run --target uploadfs`

5. **Serial monitor for IP**
    - Device prints IP address on startup
    - Use that IP directly

### No Nearby Stops Found

**Symptoms:**

- Configuration page shows "No stops found"
- Can't select transport station

**Solutions:**

1. **Verify location**
    - MyStation uses Google Geolocation
    - Check API key is configured
    - Verify internet connection

2. **Check RMV coverage**
    - RMV covers German public transport
    - Must be in Germany or covered area
    - Try manual location entry

3. **API key issues**
    - Google Geolocation API key required
    - RMV API key required
    - See [API Configuration](station-configuration.md#api-keys)

4. **Check API limits**
    - Free tier has request limits
    - May be temporarily blocked
    - Wait and try again

### Settings Don't Save

**Symptoms:**

- Change settings, but they reset
- Configuration reverts after restart

**Solutions:**

1. **Click "Save" button**
    - Must explicitly save
    - Look for confirmation message

2. **NVS storage full**
    - Rare, but possible
    - Try factory reset
    - Clears old data

3. **NVS write errors**
    - Check serial monitor for errors
    - May indicate flash memory issue

4. **Power loss during save**
    - Don't disconnect power while saving
    - Wait for confirmation message

---

## Data & API Issues

### Weather Data Not Showing

**Symptoms:**

- Display shows "---" for weather
- Temperature missing
- Weather icon blank

**Solutions:**

1. **Check location configured**
    - Weather requires valid coordinates
    - Verify in configuration page

2. **DWD API access**
    - Check internet connection
    - Verify DWD service online
    - Serial monitor shows API errors

3. **Location outside Germany**
    - DWD covers Germany only
    - Won't work in other countries
    - Consider alternative weather API

4. **Wait for next update**
    - Data might be loading
    - Check update interval setting
    - Monitor serial output

### Transport Departures Not Showing

**Symptoms:**

- No departures listed
- Empty departure board
- Only weather shows

**Solutions:**

1. **Check station configured**
    - Must select transport stop
    - Verify in configuration page

2. **RMV API access**
    - Check internet connection
    - Verify RMV API key
    - Check API limits not exceeded

3. **No departures available**
    - Late night/early morning
    - Check if service actually running
    - Try different time of day

4. **Transport filter too restrictive**
    - Check which transport types enabled
    - Enable more types (Bus, Tram, S-Bahn, etc.)

### "API Error" or "HTTP Error" Messages

**Symptoms:**

- Serial monitor shows HTTP errors
- 401, 403, 404, 500 errors

**Solutions:**

1. **Check API keys**
    - Verify keys are correct
    - Check quotas not exceeded
    - API might be temporarily down

2. **Check internet connection**
    - Verify WiFi connected
    - Test other internet on same network
    - Router might block HTTPS

3. **Certificate issues**
    - HTTPS requires valid certificates
    - Check system time is correct
    - May need to update certificates

4. **API service outage**
    - Check API status pages
    - Wait and retry later
    - May be scheduled maintenance

---

## Firmware & Update Issues

### OTA Update Fails

**Symptoms:**

- Update starts but doesn't complete
- Device restarts without updating
- "Update failed" message

**Solutions:**

1. **Check WiFi connection**
    - Strong signal required for update
    - Don't interrupt during update
    - Ensure stable connection

2. **Check battery level**
    - Low battery can cause update failure
    - Charge or connect USB during update

3. **Storage space**
    - OTA partition must have space
    - Check partition table configuration

4. **Manual update**
    - Upload firmware via USB instead
    - `pio run --target upload`

### Can't Upload Firmware

**Symptoms:**

- PlatformIO upload fails
- "Could not open port" error
- Upload timeout

**Solutions:**

1. **Check USB connection**
    - Try different cable
    - Try different USB port
    - Ensure cable supports data (not charge-only)

2. **Check drivers**
    - ESP32 requires CP210x or CH340 drivers
    - Install drivers for your OS
    - Verify device appears in device manager

3. **Close serial monitor**
    - Can't upload while monitor open
    - Close all serial connections
    - Try upload again

4. **Put board in download mode**
    - Some boards need manual boot mode
    - Hold BOOT button, press RESET
    - Try upload while BOOT held

5. **Check board definition**
    - Verify correct board selected in platformio.ini
    - ESP32-C3 vs ESP32-S3

---

## General Troubleshooting Steps

When you encounter any issue:

### 1. Check Serial Monitor

```bash
pio device monitor
```

- Most issues show error messages here
- Look for exceptions, warnings, errors

### 2. Power Cycle

- Disconnect USB and battery
- Wait 10 seconds
- Reconnect power
- Many issues resolved by restart

### 3. Verify Configuration

- Check all settings in web interface
- WiFi network (must be 2.4 GHz!)
- Station selection
- API keys
- Update interval

### 4. Check WiFi

- Verify 2.4 GHz network
- Signal strength adequate
- Router not blocking device
- Internet connection working

### 5. Update Firmware

- Ensure you have latest code
- Rebuild and upload
- Upload filesystem

```bash
git pull
pio run --target upload
pio run --target uploadfs
```

### 6. Factory Reset

- Last resort for configuration issues
- See [Factory Reset](factory-reset.md)
- Be prepared to reconfigure

### 7. Check Hardware

- Inspect all solder joints
- Verify wiring matches pin mappings
- Test continuity with multimeter
- Check for loose connections

---

## Still Need Help?

### Gather Information

Before asking for help, collect:

- Serial monitor output (full startup sequence)
- Board type (ESP32-C3 or ESP32-S3)
- Firmware version
- Description of problem
- What you've tried

### Check Documentation

- 📖 [User Guide](index.md) - Complete user documentation
- 🔧 [Developer Guide](../developer-guide/index.md) - Technical details
- 📚 [Configuration Reference](../reference/configuration-keys.md) - All settings explained

### Report Issues

If you found a bug:

- Check GitHub issues for similar problems
- Create new issue with details
- Include serial monitor output
- Describe steps to reproduce

---

**Remember**: Most issues are related to WiFi (2.4 GHz only!), wiring, or configuration. Start with the basics and work
through systematically.

