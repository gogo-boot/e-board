# E-Paper Display Layout Overview

<!-- TODO: Add photo of device showing Half & Half mode -->
![Half and Half display mode](/img/IMG_0872.jpeg)

## Display Hardware Specifications

- **Display Model**: 7.5 inch E-Paper Display GDEY075T7
- **Physical Resolution**: 800x480 pixels
- **Color Support**: Black & White (2-color), No Gray levels

## Layout Orientations

### Landscape Mode (800x480)

```
┌────────────────────────────────────┬─────────────────────────────────────────┐
│                                    │                                         │
│         WEATHER SECTION            │          DEPARTURE SECTION              │
│           (400x480)                │            (399x480)                    │
│                                    │                                         │
│  • City/Town Name: 22px            │  • Station Name: 17px                   │
│  • Space 20px                      │  • Space 10px                           │
│  • Day weather Info: 80px          │  • Departure Column Headers: 12px       │
│    - first column                  │  • Space 4px                            │
│       - Day Weather Icon: 48px     │  • Line 1px                             │
│        - Current Temp : 30 px      │     Left 421px for depature Entries     │
│    - second column                 │      - Depature Entries 42 px 5 times   │
│       - today low/high temp: 27px  │      - Separation Line 1px              │
│       - UV Index info: 20 px       │      - Depature Entries 42 px 5 times   │
│       - Pollen Info : 20px         │      Single Daparture Entry             │
│    - third column                  │        - Space 3px                      │
│       - Sunrise : 40 px            │        - Main Line: 17px                │
│       - Sunset : 40px              │        - Space 3px                      │
│  • Space 12px                      │        - Disruption Space: 16px         │
│  • Nächste Stunden 15px            │       - Space 3px                       │
│  • Space 25px                      │   • Footer Separation Line  1px         │
│  • Weather Graphic : 304px         │   • Footer: 15px                        │
│  • Space 12px                      │                                         │
│  • Footer: 15px                    │                                         │
└────────────────────────────────────┴─────────────────────────────────────────┘
```

## Update Performance

- **Full Screen**: Complete redraw (~2-3 seconds)
- **Partial Updates**: Not possible with deep sleep (see below)

---

## Weather Data in RTC Memory

### Why Weather Is Cached in RTC

```
Wake Cycle 1: Fetch Weather + Fetch Transport → Display → Sleep
Wake Cycle 2: RTC Weather (cached) + Fetch Transport → Display → Sleep
Wake Cycle 3: RTC Weather (cached) + Fetch Transport → Display → Sleep
Wake Cycle 4: Fetch Weather (interval expired) + Fetch Transport → Display → Sleep
```

Weather data (`WeatherInfo`) is stored in RTC memory because:

- **Weather updates are infrequent** (every 1-3 hours), while transport updates are frequent (every 5-10 minutes)
- **Avoids redundant API calls** — on most wake cycles, only transport data is fetched
- **Saves power** — one fewer HTTPS request per wake cycle reduces WiFi-on time by ~1-2 seconds
- **Required for display** — the half-and-half mode needs both weather and transport data every cycle

---

## Day Browse Layout

When the user browses future forecast days (via button presses in Weather-Only mode),
the display switches to a dedicated **day browse layout** rendered by `drawDayBrowseLayout()`.
This layout replaces the standard weather view with a full-width design optimized for
showing a single future day's hourly data.

### Layout (800x480, full width)

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  Date Header + City Name                                            (22px)  │
│  "Mittwoch, 3. Sep 2025 — Frankfurt"                                       │
├──────────────────────────────────────────────────────────────────────────────┤
│  Space (8px)                                                                │
├──────────────────────────────────────────────────────────────────────────────┤
│  6-Day Forecast Row (icon + high/low per day)                       (80px)  │
│  ┌─────┐ ┌─────┐ ┌══════╗ ┌─────┐ ┌─────┐ ┌─────┐                         │
│  │ Mo  │ │ Di  │ ║ Mi ▪ ║ │ Do  │ │ Fr  │ │ Sa  │  ← highlight box on     │
│  │ ☀   │ │ ⛅  │ ║ 🌧 ▪ ║ │ ☀   │ │ ⛅  │ │ ☀   │    selected day         │
│  │24/15│ │22/14│ ║19/12▪║ │25/16│ │23/15│ │26/17│                         │
│  └─────┘ └─────┘ ╚══════╝ └─────┘ └─────┘ └─────┘                         │
├──────────────────────────────────────────────────────────────────────────────┤
│  Space (12px)                                                               │
├──────────────────────────────────────────────────────────────────────────────┤
│  Full-Width 19-Point Temperature + Rain Graph (06:00–24:00)        (340px)  │
│                                                                             │
│  °C                                               mm                       │
│  25─┐                                          ─4  (rain bars right axis)   │
│     │        ╱──╲                              ─3                           │
│  20─┤      ╱      ╲                            ─2                           │
│     │    ╱          ╲──╲                       ─1                           │
│  15─┤──╱                ╲──╲                   ─0                           │
│     └────┬────┬────┬────┬────┬────┬────┬────┤                               │
│        06:00  09:00  12:00  15:00  18:00  21:00  24:00                      │
│                                                                             │
├──────────────────────────────────────────────────────────────────────────────┤
│  Footer: battery, WiFi, last update, version                        (15px)  │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 19h vs 13h Graph

The standard weather view (day 0) shows a **13-hour graph** (current hour to +12h) in
the left half of a Half & Half layout (400px wide). The day browse layout shows a
**19-point graph** (06:00 to 24:00) spanning the **full 800px width**. The 19th point
(24:00) is the next cached day's 00:00 value, appended so the x-axis has 18 intervals
that align cleanly with 3-hour grid lines (06/09/12/15/18/21/24). On the last browsable
day (no next-day data cached), only 18 points (06:00–23:00) are shown.

| Property | Standard (Day 0) | Day Browse (Day 1–6) |
|----------|-------------------|----------------------|
| Width | 400px (left half) | 800px (full width) |
| Time range | Now to +12h (13 points) | 06:00 to 24:00 (19 points; 18 on last day) |
| Data source | Cached in RTC (`WeatherInfo`) | Cached in RTC (`dayCache`, prefetched) |
| Detail columns | Sunrise, UV, wind, pollen | Not shown (replaced by graph) |

### RTC Day Cache

The per-day hourly data is prefetched into RTC memory during the regular weather update
(`getWeatherHourlyMultiDay()` — one wide `forecast_days=N` call, sliced per day). Each
cached point is a compact `DayBrowsePoint` (12 bytes); the full cache is
`DayBrowsePoint dayCache[7][24]` (~2 KB RTC). Because all browsable days live in RTC,
a day-browse button press renders **instantly** with no WiFi round-trip. See
[Boot Process](boot-process.md) for the wake-time skip-WiFi optimization.

### `drawGraphInternal()` — Shared Helper

Both the 13h and 19h graphs are rendered by the same internal function `drawGraphInternal()`.
It accepts the data array, point count, pixel dimensions, and axis configuration. The public
`drawTemperatureAndRainGraph()` overloads (one taking `WeatherInfo` for the 13h path, one
taking a `WeatherHourlyForecast[]` for the day-browse path) provide the appropriate
parameters. This avoids duplicating the temperature curve, rain bar, and axis rendering logic.

### Fallback Behavior

If the RTC day cache is empty or the selected day is not cached (e.g. a weather model with
fewer forecast days), the display falls back to the normal weather view (day 0) using cached
RTC data. The user sees today's weather instead of a blank or error screen.

---

## Partial Display Update — Not Feasible with Deep Sleep

Partial display update would allow refreshing only the transport section while keeping the weather section untouched:

```
Full refresh:     [Weather ████████ | Transport ████████]  ← full flash, 2-3s
Partial refresh:  [Weather (unchanged) | Transport ████]  ← no flash on left, <1s
```

**Why it doesn't work:**

The e-paper controller (GDEY075T7) supports `setPartialWindow()` and GxEPD2 provides the API.
U8g2_for_Adafruit_GFX also works correctly within partial windows (renders via `drawPixel()`
which GxEPD2 clips to the window). The x=400 half-width boundary is 8-pixel aligned as required.

However, **partial updates require the controller to retain the previous image** in its RAM
to calculate pixel transitions. After ESP32 deep sleep, the controller's RAM content becomes
unreliable (even with `display.hibernate()`), causing:

- Ghosting on the unchanged half (weather)
- Progressively unreadable content after multiple partial cycles
- Full-screen flash fallback on some updates

Since the device deep-sleeps between every update cycle (~5 min), there is no "second update
within the same wake cycle" where partial refresh could work. The 48 KB framebuffer
(800×480 / 8) also exceeds the ESP32's 8 KB RTC RAM, so it cannot be persisted across sleep.

**Conclusion:** Full-screen refresh is the only reliable approach for a deep-sleep device.
This is a hardware limitation, not a software one.
