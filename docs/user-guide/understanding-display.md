# Understanding the Display

This guide explains what information is shown on your MyStation e-paper display and where to find it.

## Display Layout

MyStation offers three display modes, each optimized for different needs:

1. **Half & Half Mode** - Weather and departures split view
2. **Weather Only Mode** - Full-screen weather information
3. **Departures Only Mode** - Full-screen transport departures

## Display Mode 1: Half & Half (Default)

```
┌───────────────────────────────────────┐
│ Header Bar                            │
│ Station Name          Last Update     │
├───────────────────────────────────────┤
│ WEATHER SECTION (Top Half)            │
│  ┌─────────────────┐                  │
│  │ Current Weather │ Temperature Graph│
│  │ Icon: ☀️         │  24hr forecast   │
│  │ 22°C (feels 24°)│                  │
│  └─────────────────┘                  │
├───────────────────────────────────────┤
│ DEPARTURE SECTION (Bottom Half)       │
│  🚂 RE 5  Frankfurt    Gl.3   now     │
│  🚊 S6    Friedberg    Gl.4   2 min   │
│  🚌 41    Bahnhof      C     5 min    │
│  🚂 RE 30 Königstein   Gl.2   +3 min  │
├───────────────────────────────────────┤
│ Footer: Battery 85% | WiFi: Connected │
└───────────────────────────────────────┘
```

### Information Displayed

**Top Section (Weather)**:

- Current temperature
- "Feels like" temperature
- Weather icon
- Weather description
- 24-hour temperature forecast graph

**Bottom Section (Departures)**:

- Transport type icon (🚂 RE, 🚊 S-Bahn, 🚌 Bus)
- Line number/name
- Destination
- Platform/track number
- Departure time or delay

**Footer**:

- Battery level (ESP32-S3 only)
- WiFi status
- Last update timestamp

## Display Mode 2: Weather Only

```
┌───────────────────────────────────────┐
│ Header Bar                            │
│ Location Name         Last Update     │
├───────────────────────────────────────┤
│                                       │
│          Current Conditions           │
│                                       │
│              ☀️                       │
│           Sunny                       │
│                                       │
│          22°C                         │
│      (Feels like 24°C)                │
│                                       │
├───────────────────────────────────────┤
│     24-Hour Temperature Forecast      │
│   Temp                                │
│    25°├─╮                             │
│    20°│  ╰─╮                          │
│    15°│     ╰──╮                      │
│    10°│        ╰────                  │
│      └────────────────────────────    │
│       Now 6h 12h 18h 24h              │
├───────────────────────────────────────┤
│  Additional Information               │
│  🌅 Sunrise: 06:30  🌇 Sunset: 20:15  │
│  💧 Humidity: 65%   🌬️ Wind: 12 km/h │
│  ☔ Rain: 0%        ☁️ Clouds: 20%   │
└───────────────────────────────────────┘
```

### Information Displayed

**Main Area**:

- Large weather icon
- Weather description
- Current temperature
- "Feels like" temperature
- Temperature graph (24-hour forecast)

**Additional Details**:

- Sunrise and sunset times
- Humidity percentage
- Wind speed and direction
- Precipitation probability
- Cloud coverage

**Best For**:

- Weather planning
- Outdoor activities
- Daily weather overview

## Display Mode 3: Departures Only

```
┌───────────────────────────────────────┐
│ Header Bar                            │
│ Frankfurt Hauptbahnhof Last Update    │
├───────────────────────────────────────┤
│                                       │
│  🚂 RE 5      Frankfurt Süd    Gl.3   │
│               → via Offenbach         │
│               Departing: now          │
│                                       │
│  🚊 S6        Friedberg        Gl.4   │
│               → via Bad Vilbel        │
│               Departing: in 2 min     │
│                                       │
│  🚌 41        Bahnhof Nord      C     │
│               → via Zentrum           │
│               Departing: in 5 min     │
│                                       │
│  🚂 RE 30     Königstein       Gl.2   │
│               → via Höchst            │
│               Delayed: +3 min         │
│                                       │
│  🚊 S1        Wiesbaden        Gl.5   │
│               → via Mainz             │
│               Departing: in 12 min    │
│                                       │
└───────────────────────────────────────┘
```

### Information Displayed

**For Each Departure**:

- Transport type icon
- Line number or name
- Final destination
- Key intermediate stops (via)
- Platform or track number
- Departure time
- Delay information (if any)

**Transport Types**:

- 🚂 **RE** - Regional Express
- 🚊 **S** - S-Bahn (suburban rail)
- 🚆 **RB** - Regional train
- 🚌 **Bus** - Local bus
- 🚎 **Tram** - Tram/Streetcar
- 🚇 **U** - U-Bahn (metro)

**Time Display**:

- "now" - Departing immediately
- "in X min" - Minutes until departure
- "+X min" - Delayed by X minutes
- Exact time (e.g., "14:35")

**Best For**:

- Commuting
- Planning trips
- Checking multiple connections

## Display Elements Explained

### Header Bar

**Left Side**: Station or location name

- In Weather mode: City/location name
- In Departure mode: Transport station name
- In Half & Half: Station name

**Right Side**: Last update timestamp

- Format: "HH:MM" (24-hour format)
- Shows when data was last refreshed
- Helps verify information is current

### Weather Icons

MyStation uses intuitive weather icons:

| Icon | Meaning       |
|------|---------------|
| ☀️   | Clear/Sunny   |
| ⛅    | Partly Cloudy |
| ☁️   | Cloudy        |
| 🌧️  | Rain          |
| ⛈️   | Thunderstorm  |
| 🌨️  | Snow          |
| 🌫️  | Fog           |
| 🌬️  | Windy         |

### Temperature Graph

**24-Hour Forecast**:

- X-axis: Time (now, +6h, +12h, +18h, +24h)
- Y-axis: Temperature in °C
- Line shows temperature trend
- Helps plan for temperature changes

**Reading the Graph**:

- Rising line: Temperature increasing
- Falling line: Temperature decreasing
- Steep changes: Rapid temperature shifts
- Flat line: Stable temperature

### Departure Information

**Transport Icons**:

- Visual identification of transport type
- Helps quickly find desired service
- Color-coded in original code (grayscale on e-paper)

**Delay Indicators**:

- Green: On time
- Yellow: Minor delay (1-5 minutes)
- Red: Major delay (> 5 minutes)
- Gray: No real-time data

**Platform/Track**:

- "Gl.X" - Gleis (German for platform/track)
- "Bus X" - Bus bay/stand
- "A", "B", "C" - Platform sections

### Footer Information

**Battery Level** (ESP32-S3 only):

- Shows remaining battery percentage
- Icons: 🔋 (full), 🪫 (low)
- Percentage: 0-100%
- Warning when < 20%

**WiFi Status**:

- "Connected" - WiFi working
- "Disconnected" - WiFi issue
- Signal strength indicator

**Update Time**:

- When data was last fetched
- Helps verify freshness

## Reading the Display

### Quick Glance (5 seconds)

**Half & Half Mode**:

1. Check time in header (is data fresh?)
2. Scan weather icon and temperature
3. Look at first 1-2 departures
4. Note any delays (red text)

**Weather Mode**:

1. Current temperature (big number)
2. Weather icon (what to expect)
3. Graph trend (warming/cooling)

**Departures Mode**:

1. Find your line
2. Check departure time
3. Note platform number
4. Check for delays

### Detailed Reading (30 seconds)

**Weather Information**:

- Compare "feels like" to actual temperature
- Check 24-hour trend for planning
- Note sunrise/sunset for daylight planning
- Check rain probability before going out
- Wind speed for outdoor activities

**Departure Information**:

- Review all available departures
- Compare different route options
- Note delays for time planning
- Check intermediate stops (via)
- Verify platform numbers

## Update Behavior

### How Often Display Updates

**Configured Interval** (default: 5 minutes):

- Device wakes from sleep
- Connects to WiFi (2.4 GHz)
- Fetches latest data
- Updates display
- Returns to sleep

**Why Not Continuous?**:

- Saves battery life
- E-paper doesn't need constant refresh
- Information doesn't change that quickly
- 5-minute updates provide good balance

### Display Refresh Process

1. **Wake from Sleep** (< 1 second)
2. **Connect to WiFi** (5-10 seconds)
3. **Fetch Weather Data** (2-5 seconds)
4. **Fetch Departure Data** (2-5 seconds)
5. **Render Display** (30-45 seconds)
    - E-paper refresh is slow (this is normal!)
    - You'll see the display flash/flicker
    - Final image appears after refresh
6. **Enter Deep Sleep** (until next update)

**Total cycle time**: ~40-70 seconds

### E-Paper Characteristics

**Normal Behavior**:

- ✅ Display "flickers" during refresh (black/white flash)
- ✅ Takes 30-45 seconds to fully update
- ✅ Image persists without power
- ✅ Slight "ghosting" from previous image

**Not Normal**:

- ❌ Display stays blank
- ❌ Corrupted/garbled image
- ❌ Never refreshes
- ❌ Only partial update

See [Troubleshooting](troubleshooting.md#display-issues) if you experience issues.

## Customizing the Display

### Change Display Mode

**Via Web Interface**:

1. Access `http://mystation.local`
2. Navigate to Display Settings
3. Select mode:
    - Half & Half
    - Weather Only
    - Departures Only
4. Click "Save Settings"
5. Device will use new mode on next update

**Via Buttons** (ESP32-S3 only):

- Button 1: Half & Half (temporary)
- Button 2: Weather Only (temporary)
- Button 3: Departures Only (temporary)

See [Button Controls](button-controls.md) for details.

### Update Interval

**Options**: 1-60 minutes

**Recommendations**:

- **1-3 minutes**: Very current data, short battery life
- **5 minutes** (default): Good balance
- **10-15 minutes**: Less frequent updates, longer battery life
- **30-60 minutes**: Maximum battery life

**Configure**:

1. Access web interface
2. Go to Settings
3. Set "Update Interval"
4. Save

### Sleep Schedule

**Purpose**: Save battery during quiet hours

**Example**:

- Sleep: 23:00 - 06:00
- No updates overnight
- Automatically resumes in morning

**Configure**:

1. Access web interface
2. Enable "Sleep Schedule"
3. Set start and end times
4. Save

## Tips for Best Experience

### Optimal Viewing

✅ **Good conditions**:

- Indoor lighting
- Outdoor shade
- Indirect sunlight

❌ **Difficult conditions**:

- Direct bright sunlight (can wash out display)
- Complete darkness (no backlight)
- Extreme angles

### Interpreting Data

**Weather**:

- Check update time - weather changes
- "Feels like" more relevant than actual temp
- Graph shows trends, not exact values
- DWD data specific to Germany

**Departures**:

- Real-time data can have delays
- Early morning/late night may show no departures
- Delays shown are estimates
- Platform changes possible

### Battery Life Optimization

To maximize battery life while keeping useful information:

1. **Set reasonable update interval**
    - 10 minutes: Still very current
    - 15 minutes: Good for most uses
    - 5 minutes only if you need real-time data

2. **Use sleep schedule**
    - No updates overnight saves 30-40% battery
    - Most people don't need updates while sleeping

3. **Choose efficient display mode**
    - Weather Only: Fewer API calls
    - Half & Half: Balanced
    - Departures Only: More API calls (depends on config)

4. **Strong WiFi signal**
    - Place device near router (but still visible)
    - Weak signal uses more power

## Related Documentation

- 🏠 [User Guide Home](index.md)
- ⚙️ [Station Configuration](station-configuration.md)
- 🔘 [Button Controls](button-controls.md)
- 🔋 [Battery & Charging](battery-charging.md)
- 🔧 [Troubleshooting](troubleshooting.md)

---

**Remember**: The e-paper display retains its image even when powered off. This is normal e-paper behavior and helps
save battery!

