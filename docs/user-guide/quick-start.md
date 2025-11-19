# Quick Start Guide

Get your MyStation E-Board up and running in 15 minutes!

## Prerequisites Checklist

Before you begin, make sure you have:

- [ ] ESP32-C3 Super Mini or ESP32-S3 board
- [ ] 7.5" e-paper display (GDEY075T7, 800x480)
- [ ] USB-C cable
- [ ] Computer with PlatformIO installed
- [ ] WiFi network (2.4 GHz only - **5 GHz is not supported**)
- [ ] Internet connection

> ⚠️ **Important**: MyStation only works with **2.4 GHz WiFi networks**. 5 GHz networks are not supported due to higher
> energy consumption requirements.

## Step 1: Hardware Assembly (5 minutes)

### Connect the Display

Connect your e-paper display to the ESP32 board following the pin mapping:

**For ESP32-C3 Super Mini:**

| E-Paper Pin | ESP32-C3 Pin | GPIO |
|-------------|--------------|------|
| BUSY        | A2           | 2    |
| CS          | A3           | 3    |
| SCK         | SCK          | 4    |
| SDI         | MOSI         | 6    |
| RES         | SDA          | 8    |
| DC          | SCL          | 9    |
| 3.3V        | 3.3V         | -    |
| GND         | GND          | -    |

**For ESP32-S3:**

| E-Paper Pin | ESP32-S3 Pin | GPIO |
|-------------|--------------|------|
| BUSY        | GPIO 4       | 4    |
| CS          | GPIO 44      | 44   |
| SCK         | GPIO 7       | 7    |
| SDI         | GPIO 9       | 9    |
| RES         | GPIO 38      | 38   |
| DC          | GPIO 10      | 10   |
| 3.3V        | 3.3V         | -    |
| GND         | GND          | -    |

> 📖 Need detailed wiring instructions? See [Hardware Assembly](hardware-assembly.md)

## Step 2: Install Firmware (5 minutes)

### Build and Upload

```bash
# Clone the repository
git clone <repository-url>
cd mystation

# Build and upload firmware
pio run --target upload

# Upload web interface files
pio run --target uploadfs
```

### Monitor Startup

Open the serial monitor to watch the boot process:

```bash
pio device monitor
```

You should see:

```
[MAIN] System starting...
[MAIN] Fresh boot: No configuration found
[MAIN] Starting WiFi configuration mode...
[MAIN] AP SSID: MyStation-XXXXXXXX
```

## Step 3: WiFi Configuration (3 minutes)

### Connect to MyStation

1. **Find the WiFi network** named `MyStation-XXXXXXXX` (XXXXXXXX is your device's unique ID)
2. **Connect** using your phone or computer
3. **No password required** for initial setup

### Configure WiFi

1. **Open your browser** and go to `http://10.0.1.1`
2. **Select your home WiFi** network from the list
    - ⚠️ **Make sure it's a 2.4 GHz network** (5 GHz will not work)
3. **Enter your WiFi password**
4. **Click "Save"**

The device will:

- Connect to your WiFi network
- Get an IP address
- Start the configuration web interface
- Detect your location automatically

> 💡 **Tip**: After connecting to WiFi, the serial monitor will show your device's IP address

## Step 4: Station Configuration (5 minutes)

### Access Configuration Interface

After WiFi setup, access the configuration page:

- **By IP**: `http://192.168.1.XXX` (check serial monitor for the exact IP)
- **By mDNS**: `http://mystation.local` (if your device supports mDNS)

### Configure Your Location

The device will automatically:

1. ✅ Detect your location using Google Geolocation API
2. ✅ Find nearby public transport stops
3. ✅ Display available stops in the configuration interface

### Select Your Transport Stop

1. **Review the list of nearby stops** shown on the configuration page
2. **Select your preferred departure station**
3. **Choose transport types** you want to see (RE, S-Bahn, Bus, etc.)

### Display Settings

Configure how and when to update:

- **Update Interval**: How often to refresh data (default: 5 minutes)
    - Shorter intervals = more current data, less battery life
    - Recommended: 5-15 minutes for daily use

- **Display Mode**: What to show on screen
    - Half & Half: Weather + Departures split screen
    - Weather Only: Full screen weather display
    - Departures Only: Full screen departure display

- **Sleep Schedule** (Optional): Set quiet hours
    - Example: Sleep from 23:00 to 06:00
    - Saves battery overnight

### Save Configuration

Click **"Save Settings"** and your device will:

1. Save all settings to permanent storage
2. Restart in operational mode
3. Connect to WiFi automatically
4. Fetch and display data
5. Enter deep sleep until next update

## Step 5: Verify Operation (2 minutes)

### Check the Display

Your e-paper display should now show:

- 🌤️ Current weather information
- 🚌 Upcoming departures from your selected stop
- ⏰ Last update timestamp
- 🔋 Battery status (ESP32-S3 only)

### Understand the Update Cycle

MyStation operates in cycles:

1. **Wake up** at scheduled time
2. **Connect to WiFi** (2.4 GHz network)
3. **Fetch data** from APIs
4. **Update display** with new information
5. **Enter deep sleep** until next update

This cycle minimizes power consumption while keeping information current.

## What's Next?

### Daily Usage

Your MyStation is now fully configured! It will:

- ✅ Wake up automatically at your configured interval
- ✅ Connect to your 2.4 GHz WiFi network
- ✅ Update weather and departure information
- ✅ Go back to sleep to save battery
- ✅ Retain all settings even if power is lost

### Optional: Button Controls (ESP32-S3 Only)

If you have an ESP32-S3 with buttons connected, you can:

- Press Button 1: Show Weather + Departures
- Press Button 2: Show Weather Only
- Press Button 3: Show Departures Only

See [Button Controls](button-controls.md) for details.

### Need to Reconfigure?

To change settings later:

1. Connect to `http://mystation.local` or your device's IP address
2. Update your preferences
3. Click "Save Settings"

Or perform a [Factory Reset](factory-reset.md) to start fresh.

## Common First-Time Issues

### "Cannot connect to WiFi network"

- ✅ Verify you're connecting to a **2.4 GHz network** (not 5 GHz)
- ✅ Check that WiFi password is correct
- ✅ Make sure signal strength is adequate

### "No nearby stops found"

- ✅ Verify internet connection is working
- ✅ Check that Google Geolocation API key is configured
- ✅ Ensure you're in an area covered by RMV (German public transport)

### "Display not updating"

- ✅ Check serial monitor for error messages
- ✅ Verify WiFi connection is stable
- ✅ Ensure API keys are properly configured

For more help, see the [Troubleshooting Guide](troubleshooting.md).

## Battery Installation

### Connect Battery

1. **Power off** the device
2. **Connect LiPo battery** to the JST connector (if available)
3. **Power on** and verify operation
4. **Unplug USB** for battery-only operation

### Battery Life Expectations

**ESP32-C3 Super Mini:**

- 1000mAh battery: 1-2 weeks (5-minute updates)
- 2000mAh battery: 2-4 weeks (5-minute updates)

**ESP32-S3:**

- Similar to C3, with battery voltage monitoring

> 💡 **Tip**: Increase update interval to 10-15 minutes for longer battery life

## Next Steps

- 📖 [Understanding the Display](understanding-display.md) - Learn what information is shown
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions

Congratulations! Your MyStation is now running! 🎉

# MyStation E-Board - User Guide

Welcome to MyStation, your personal public transport departure board powered by ESP32!

## What is MyStation?

MyStation is a battery-powered e-paper display that shows real-time public transport departures and weather information
for your location. It's designed to be:

- 🚌 **Always Up-to-Date** - Displays real-time German public transport (RMV) departures
- 🌤️ **Weather-Aware** - Shows current weather and forecasts from German Weather Service (DWD)
- 🔋 **Ultra Low Power** - Runs for 2-14 days on a single battery charge with intelligent sleep modes
- 📱 **Easy to Configure** - Simple web interface for setup and customization
- 🔒 **Privacy-Focused** - All data processing happens locally on your device
- 📡 **WiFi Connected** - Automatic updates over 2.4 GHz WiFi networks

## Key Features

### Real-Time Departures

- Display departures from your favorite public transport stops
- Filter by transport type (RE, S-Bahn, Bus, Tram, etc.)
- See departure times, delays, and platform information
- Multiple display modes: weather only, departures only, or combined

### Weather Information

- Current weather conditions
- Temperature and "feels like" temperature
- Weather icons and descriptions
- 24-hour forecast graphs

### Smart Power Management

- Intelligent deep sleep between updates
- Configurable update intervals (1-60 minutes)
- Optional sleep schedule (e.g., sleep overnight)
- Battery monitoring (ESP32-S3 only)

### Easy Configuration

- WiFi access point for initial setup
- Mobile-friendly web interface
- Automatic location detection
- Nearby stop discovery
- Over-the-air (OTA) firmware updates

## Hardware Variants

### ESP32-C3 Super Mini (Current)

- Compact design
- 2-4 weeks battery life (2000mAh battery, 5-minute updates)
- All core features supported
- No physical buttons

### ESP32-S3 (Advanced)

- 3 physical buttons for display mode switching
- Battery voltage monitoring
- Slightly better performance
- All ESP32-C3 features plus button controls

## What You'll Need

### Required Hardware

- ESP32-C3 Super Mini or ESP32-S3 development board
- 7.5" e-paper display (800x480, GDEY075T7)
- USB-C cable for programming and charging
- 3.7V LiPo battery (1000-2500mAh recommended)

### Required Software & Services

- PlatformIO for firmware installation (first-time only)
- WiFi network (2.4 GHz - **5 GHz is not supported**)
- Internet connection for data updates

### Optional API Keys

For full functionality, you'll need free API keys:

- **Google Geolocation API** - For automatic location detection
- **RMV API** - For German public transport data

> 💡 Don't worry! The setup guide will walk you through obtaining these keys.

## Getting Started

Ready to set up your MyStation? Head to the [Quick Start Guide](quick-start.md) to get your device running in 15
minutes!

## Need Help?

- 🚀 [Quick Start Guide](quick-start.md) - First-time setup
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions
- 🔄 [Factory Reset](factory-reset.md) - Reset to defaults
- 📖 [Full Documentation](../index.md) - Complete documentation index

