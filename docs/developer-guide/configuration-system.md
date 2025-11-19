# Configuration Guide

Detailed configuration options for the MyStation E-Board web interface.

## Accessing Configuration

## Accessing Configuration

### Device Boot States
The device operates in two main states:

#### Configuration Mode
Triggered when:
- **First boot** (no saved configuration found)
- **Configuration corruption** (invalid settings detected)
- **Manual trigger** (specific button combination - if implemented)
- **Explicit reset** (via web interface factory reset)

In configuration mode:
- Creates WiFi access point (`MyStation-XXXXXXXX`)
- Starts web server on `http://10.0.1.1`
- Waits for user configuration
- Device stays awake (no deep sleep)

#### Operational Mode  
Normal operation after configuration:
- Loads settings from permanent storage (NVS)
- Connects to saved WiFi network
- Fetches and displays data
- Enters deep sleep between updates
- No web interface (power saving)

### Configuration Persistence
All settings are stored in **Non-Volatile Storage (NVS)** which:
- ✅ Survives power loss and battery changes
- ✅ Persists through firmware updates  
- ✅ Maintains settings across reboots
- ✅ Only requires configuration once

### Web Interface Access
- **Initial Setup**: `http://10.0.1.1` (configuration mode)
- **Local Network**: `http://[device-ip]` (check serial monitor)
- **mDNS**: `http://mystation.local` (if supported by your network)

## Configuration Sections

### 1. Location Settings

#### Automatic Location Detection
- **Enabled by default** when Google API key is configured
- **Uses WiFi positioning** for accuracy without GPS
- **Fallback to IP-based** location if WiFi positioning fails

#### Manual Location Override
```html
Latitude:  [  50.1109  ] (decimal degrees)
Longitude: [   8.6821  ] (decimal degrees)
City:      [ Frankfurt ] (auto-populated)
```

**When to use manual override:**
- Automatic detection is inaccurate
- Want to monitor a different location
- Privacy concerns with automatic detection

#### Location Accuracy
- **Automatic**: Usually within 100-500m
- **Manual**: Exact coordinates you specify
- **Impact**: Affects nearby stop discovery and weather data

---

### 2. Transport Configuration

#### Stop Selection
The interface shows nearby stops in order of distance:

```html
📍 Nearby Public Transport Stops:
┌─────────────────────────────────────────┐
│ ○ Frankfurt Hauptbahnhof         (120m) │
│ ○ Frankfurt Taunusanlage         (280m) │  
│ ○ Frankfurt Hauptwache           (450m) │
│ ○ Frankfurt Konstablerwache      (650m) │
└─────────────────────────────────────────┘
```

**Selection Options:**
- **Primary Stop**: Main departure point (always shown)
- **Secondary Stop**: Optional alternative (shown if space permits)
- **Auto-rotation**: Cycle between multiple stops

#### Transport Type Filters
Choose which transport types to display:

```html
🚊 Transport Type Filters:
┌─────────────────────────────────────────┐
│ ☑ RE (Regional Express)                 │
│ ☑ S-Bahn (Urban Rail)                   │
│ ☑ Bus (Local/Regional Bus)              │
│ ☐ U-Bahn (Underground/Metro)            │
│ ☐ Tram (City Tram)                      │
│ ☐ IC/ICE (Long Distance)                │
└─────────────────────────────────────────┘
```

**Filter Benefits:**
- **Reduce clutter**: Hide irrelevant transport
- **Focus on commute**: Show only your regular routes
- **Performance**: Fewer API calls, faster updates

#### Advanced Transport Settings
```html
Departure Limit:     [ 5 ] departures per update
Time Horizon:        [ 60 ] minutes ahead
Include Delays:      [☑] Show real-time delays
Walking Time:        [ 3 ] minutes to stop
```

---

### 3. Display Configuration

#### Update Intervals
Choose how often to refresh data:

```html
🕒 Update Schedule:
┌─────────────────────────────────────────┐
│ ○ Every 1 minute  (high power usage)    │
│ ● Every 5 minutes (recommended)         │
│ ○ Every 10 minutes (longer battery)     │
│ ○ Every 15 minutes (maximum battery)    │
│ ○ Custom interval: [___] minutes        │
└─────────────────────────────────────────┘
```

**Power Impact:**
- **1 minute**: ~8-12 hours battery life
- **5 minutes**: ~2-4 weeks battery life
- **15 minutes**: ~6-8 weeks battery life

#### Sleep Schedule (Optional)
Set quiet hours to extend battery life:

```html
🌙 Night Mode Schedule:
┌─────────────────────────────────────────┐
│ [☑] Enable night mode                   │
│ Sleep from: [23:00] until [06:00]       │
│ Night interval: [60] minutes            │
└─────────────────────────────────────────┘
```

**Benefits:**
- **Extended battery**: Reduce updates when not needed
- **Intelligent wake**: Resume normal schedule automatically

#### Display Layout Options
```html
📱 Display Layout:
┌─────────────────────────────────────────┐
│ ● Compact (3-4 departures + weather)    │
│ ○ Detailed (2-3 departures + details)   │
│ ○ Weather Focus (1-2 departures + full) │
│ ○ Minimal (departures only)             │
└─────────────────────────────────────────┘
```

---

### 4. Weather Configuration

#### Weather Display Options
```html
🌤️ Weather Information:
┌─────────────────────────────────────────┐
│ [☑] Show current weather                │
│ [☑] Include temperature                 │
│ [☑] Show weather icon                   │
│ [☐] Include humidity                    │
│ [☐] Show precipitation probability      │
│ [☐] Include wind information            │
└─────────────────────────────────────────┘
```

#### Weather Update Schedule
```html
Weather Refresh:
┌─────────────────────────────────────────┐
│ ○ Every hour (maximal updates)          │
│ ● Every 3 hours (recommended)           │
│ ○ Every day (minimal updates)           │
└─────────────────────────────────────────┘
```

---

### 5. Power Management

#### Deep Sleep Configuration
```html
🔋 Power Management:
┌─────────────────────────────────────────┐
│ Deep Sleep: [☑] Enabled                 │
│ Wake Strategy:                          │
│ ● Interval-based (every N minutes)      │
│ ○ Time-based (specific times)           │
│ ○ Hybrid (interval + night mode)        │
└─────────────────────────────────────────┘
```

#### Power Monitoring
```html
⚡ Power Status:
┌─────────────────────────────────────────┐
│ Supply Voltage: 3.28V                   │
│ Current Draw: 45mA (active)             │
│ Sleep Current: <50µA                    │
│ Battery Level: 78% (estimated)          │
│                                         │
│ Uptime: 2 days, 14 hours                │
│ Wake Count: 1,247                       │
└─────────────────────────────────────────┘
```

---

### 6. Privacy & Security

#### Data Privacy Settings
```html
🔒 Privacy Configuration:
┌─────────────────────────────────────────┐
│ [☑] Keep all data on device             │
│ [☑] No cloud data transmission          │
│ [☑] Encrypt saved configuration         │
│ [☐] Anonymous usage statistics          │
└─────────────────────────────────────────┘
```

#### API Usage Transparency
```html
📊 External API Usage:
┌─────────────────────────────────────────┐
│ Google Geolocation:                     │
│ └─ Purpose: Initial location detection  │
│ └─ Frequency: Once during setup         │
│                                         │
│ RMV Transport API:                      │
│ └─ Purpose: Departure information       │
│ └─ Frequency: Every 5 minutes           │
│                                         │
│ DWD Weather API:                        │
│ └─ Purpose: Weather information         │
│ └─ Frequency: Every 5 minutes           │
└─────────────────────────────────────────┘
```

---

### 8. System Information

#### Device Status
```html
🖥️ System Information:
┌─────────────────────────────────────────┐
│ Firmware Version: v2.1.0                │
│ Hardware: ESP32-C3 Super Mini           │
│ Flash Size: 4MB                         │
│ Free Memory: 187KB                      │
│                                         │
│ Last Boot: 2025-06-30 12:34:56         │
│ Boot Reason: Deep sleep timer           │
│ Configuration: Saved                    │
└─────────────────────────────────────────┘
```

#### Diagnostics
```html
🔍 System Diagnostics:
┌─────────────────────────────────────────┐
│ WiFi Connection: ✅ Stable              │
│ NTP Time Sync: ✅ Synchronized          │
│ API Connectivity: ✅ All services OK    │
│ Display Driver: ✅ Responding           │
│ File System: ✅ 1.2MB free             │
│                                         │
│ [Run Full Diagnostics]                  │
│ [Download Debug Log]                    │
└─────────────────────────────────────────┘
```

## Configuration Tips

### Optimal Settings
**For Maximum Battery Life:**
- Update interval: 15 minutes
- Night mode: 23:00-06:00 (60 min interval)
- Minimal display layout
- Weather updates: hourly

**For Best User Experience:**
- Update interval: 5 minutes
- No night mode restrictions
- Detailed display layout
- All weather information enabled

**For Development/Testing:**
- Update interval: 1 minute
- All debug options enabled
- Detailed logging
- Manual location override

### Performance Considerations
- **More frequent updates** = shorter battery life
- **More transport filters** = more API calls
- **Detailed display** = longer screen refresh time
- **Multiple stops** = increased data processing

## Saving Configuration

### Configuration Process
1. **Review all settings** in web interface  
2. **Click "Save Configuration"** button
3. **Wait for confirmation** (green success message)
4. **Device automatically restarts** in operational mode
5. **Monitor serial output** to verify settings applied

### Configuration Persistence
- Settings saved to **LittleFS filesystem**
- **Survives power cycles** and firmware updates
- **Encrypted storage** (if enabled)
- **Factory reset option** available

### Backup & Restore
```html
💾 Configuration Backup:
┌─────────────────────────────────────────┐
│ [Download Configuration]                │
│ [Upload Configuration]                  │
│ [Reset to Defaults]                     │
└─────────────────────────────────────────┘
```

## Next Steps
- [Deep Sleep Configuration](./deep-sleep.md) - Detailed power management
- [API Reference](./api-reference.md) - Web interface customization  
- [Troubleshooting](./troubleshooting.md) - Configuration issues
