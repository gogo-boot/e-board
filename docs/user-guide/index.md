# MyStation E-Board - User Guide

Welcome to MyStation, your personal public transport departure board!

## What is MyStation?

MyStation is a battery-powered e-paper display that shows real-time public transport departures and weather information
for your location. It's designed to be:

- 🚌 **Always Up-to-Date** - Displays real-time German public transport (RMV) departures
- 🌤️ **Weather-Aware** - Shows current weather and forecasts from German Weather Service (DWD)
- 🔋 **Ultra Low Power** - Runs for weeks on a single battery charge
- 📱 **Easy to Configure** - Simple web interface for setup
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
- Battery monitoring and status display

### Easy Configuration

- WiFi access point for initial setup
- Mobile-friendly web interface
- Automatic location detection
- Nearby stop discovery
- Over-the-air (OTA) firmware updates

## What You'll Need

### Hardware (Included with Your Device)

- MyStation device with 7.5" e-paper display
- USB-C cable for charging and initial setup
- 3.7V LiPo battery (optional, for wireless operation)

### Network Requirements

- WiFi network (2.4 GHz - **5 GHz is not supported**)
- Internet connection for data updates

> ⚠️ **Important**: MyStation only works with **2.4 GHz WiFi networks**. 5 GHz networks are not supported due to higher
> energy consumption.

### Optional API Keys

For full functionality, you'll need free API keys:

- **Google Geolocation API** - For automatic location detection
- **RMV API** - For German public transport data

> 💡 Don't worry! The setup guide will walk you through obtaining these keys.

## Device Controls

Your MyStation device includes:

- **Physical Buttons** (3 buttons) - Switch between display modes instantly
    - Button 1: Weather + Departures view
    - Button 2: Weather only view
    - Button 3: Departures only view
    - Button 1 (long press): Factory reset
- **Reset Button** - Restart the device if needed

See [Button Controls](button-controls.md) for detailed usage.

## Getting Started

Ready to set up your MyStation? Head to the [Quick Start Guide](quick-start.md) to get your device running in 15
minutes!

## User Guide Contents

### Setup & Configuration

- **[Quick Start](quick-start.md)** - Get up and running in 15 minutes
- **[Hardware Assembly](hardware-assembly.md)** - Physical setup (if building from scratch)

### Daily Usage

- **[Understanding the Display](understanding-display.md)** - What information is shown and where
- **[Button Controls](button-controls.md)** - Using the physical buttons

### Maintenance & Support

- **[Factory Reset](factory-reset.md)** - Reset to default settings
- **[Troubleshooting](troubleshooting.md)** - Common issues and solutions

## Important Notes

### WiFi Requirements

> **MyStation only works with 2.4 GHz WiFi networks.** 5 GHz networks are not supported due to higher energy consumption
> requirements.

When setting up WiFi:

- ✅ Look for your WiFi network name (SSID)
- ✅ Make sure it's a 2.4 GHz network
- ✅ Check signal strength is adequate
- ✅ Have your WiFi password ready

### Regional Support

> **Public transport data is for German transit systems** (RMV API). Weather data uses German Weather Service (DWD).

MyStation is optimized for use in Germany with access to:

- German public transport networks (via RMV)
- German weather forecasts (via DWD)

### Battery Life

Expected battery life depends on update frequency:

- **5-minute updates**: 2-4 weeks (2000mAh battery)
- **10-minute updates**: 4-8 weeks (2000mAh battery)
- **15-minute updates**: 6-12 weeks (2000mAh battery)

For longer battery life:

- Increase update interval
- Enable sleep schedule (no updates overnight)
- Ensure strong WiFi signal

## Need Help?

- 🚀 [Quick Start Guide](quick-start.md) - First-time setup
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions
- 🔄 [Factory Reset](factory-reset.md) - Reset to defaults
- 📖 [Full Documentation](../index.md) - Complete documentation index

## Quick Links

### First Time Setup

1. [Quick Start Guide](quick-start.md) - Complete setup walkthrough
2. [WiFi Configuration](quick-start.md#step-3-wifi-configuration-3-minutes) - Connect to your network
3. [Station Configuration](quick-start.md#step-4-station-configuration-5-minutes) - Select your transport stop

### Common Tasks

- **Change Display Mode** - Press Button 1, 2, or 3
- **Check Battery** - Shown in display footer
- **Update Firmware** - Automatic via OTA updates
- **Reconfigure WiFi** - Access web interface at `http://mystation.local`
- **Factory Reset** - Hold Button 1 for 5 seconds during power-on

### Troubleshooting

- **Can't connect to WiFi** - Make sure you're using a 2.4 GHz network
- **Display not updating** - Check WiFi connection and signal strength
- **Battery drains quickly** - Increase update interval or enable sleep schedule
- **Buttons not working** - See [Troubleshooting Guide](troubleshooting.md#button-issues)

For more help, see the complete [Troubleshooting Guide](troubleshooting.md).

---

**Ready to get started?** Head to the [Quick Start Guide](quick-start.md)!

