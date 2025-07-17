# E-Paper Display Layout Overview

## Display Hardware Specifications
- **Display Model**: GxEPD2_750_GDEY075T7
- **Physical Resolution**: 800x480 pixels
- **Color Support**: Black & White (2-color)
- **Technology**: E-Paper/E-Ink

## Layout Orientations

### Landscape Mode (800x480)
```
┌────────────────────────────────────┬─────────────────────────────────────────┐
│                                    │                                         │
│         WEATHER SECTION            │          DEPARTURE SECTION              │
│           (400x480)                │            (400x480)                    │
│                                    │                                         │
│  • City/Town Name: 40px            │  • Station Name: 40px                   │
│  • Day weather Info: 67px          │  • Column Headers: 30px                 │
│    - first column                  │  • Departure Entries: 36px each         │
│       - Day Weather Icon: 37px     │    - Main Line: 20px                    │
│       - Current Temp : 30 px       │    - Disruption Space: 16px             │
│    - second column                 │   • Separation Line  1px                │
│       - today low/high temp: 27px  │   • Footer: 15px                        │
│       - UV Index info: 20 px       │                                         │
│       - Pollen Info : 20px         │                                         │
│    - third column                  │                                         │
│       - Date Info : 27px           │                                         │
│       - Sunrise : 20 px            │                                         │
│       - Sunset : 20px              │                                         │
│  • Space 12px                      │                                         │
│  • Weather Graphic : 334px         │                                         │
│  • Space 12px                      │                                         │
│  • Footer: 15px                    │                                         │
└────────────────────────────────────┴─────────────────────────────────────────┘
```

## Detailed Pixel Allocations

### Weather Section
- **Landscape**: 400x480 pixels (left half, excluding header)
- **Margins**: 10px left/right, 25px top
- **Content Allocation**:
  - Title: 45px (full screen) / 30px (half screen)
  - Temperature: 40px (large font)
  - Location: 25px
  - Condition: 20px
  - High/Low: 20px
  - Forecast: Variable (16px per entry)
  - Sunrise/Sunset: 25px (bottom, if space available)

### Departure Section
- **Landscape**: 400x480 pixels (right half, excluding header)
- **Margins**: 10px left/right, 25px top
- **Content Structure**:
  - Station name: 40px
  - Column headers: 30px (18px text + 12px underline)
  - Departure entries: Variable (see below)
  - Footer: 15px (bottom)

### Footer Section
- **Height**: 15 pixels (fixed, within departure section)
- **Position**: Bottom of departure section
- **Content**: 
  - Update timestamp in German format: "Aktualisiert: HH:MM DD.MM"
  - Timezone: German local time (CET/CEST)
  - Fallback messages: "Zeit nicht synchronisiert" / "Zeit nicht verfügbar"
- **Font**: Small font (9pt)
- **Margins**: 10px left margin (aligned with departure content)

### Departure Entry Layout (Per Entry)
Each departure entry consists of:

#### Main Departure Line
- **Height**: 20px (departure info line)
- **Content**:
  - Soll time: ~25px width (5 chars + space)
  - Ist time: ~25px width (5 chars + highlighting if different)
  - Line info: Variable width (1/4 or 1/3 of remaining space)
  - Destination: Variable width (3/4 or 2/3 of remaining space)
  - Track: ~20px width (4 chars, full screen only)

#### Disruption Info Area (Always Allocated)
- **Height**: 17px (consistently allocated for all entries)
- **Indentation**: 20px from left margin
- **Content**: Warning symbol (⚠) + disruption text (if available)
- **Font**: Small font (9pt)

#### Total Per Entry
- **Total Height**: 37px (20px departure + 17px disruption space)
- **Consistent Spacing**: Same for all entries regardless of disruption info

### Maximum Entries Per Screen

#### Full Screen Mode (800px width)
- **Available height**: ~480px (480px - footer)
- **Entries per screen**: ~10 entries (400px ÷ 37px per entry)
- **Current limit**: 20 entries (code limited)

#### Half Screen Mode (400px width)
- **Available height**: ~400px
- **Entries per screen**: ~10 entries
- **Current limit**: 15 entries (code limited)

## Font Specifications

### Font Sizes Used
- **Large Font**: FreeSansBold18pt7b (18pt)
  - Used for: Weather temperature, weather title (full screen)
- **Medium Font**: FreeSansBold12pt7b (12pt)
  - Used for: Station name, weather title (half screen), weather location
- **Small Font**: FreeSansBold9pt7b (9pt)
  - Used for: Departure entries, disruption info, footer

### Text Height Approximations
- **Small Font (9pt)**: ~12px height
- **Medium Font (12pt)**: ~16px height
- **Large Font (18pt)**: ~24px height

## Color and Highlighting

### Normal Display
- **Background**: White (GxEPD_WHITE)
- **Text**: Black (GxEPD_BLACK)

### Time Highlighting (When Soll ≠ Ist)
- **Background**: Black rectangle behind Ist time
- **Text**: White text (GxEPD_WHITE) for highlighted time
- **Dimensions**: Text width × 12px height

### Visual Indicators
- **Warning Symbol**: ⚠ (for disruption info)
- **WiFi Status**: ●●● (signal strength placeholder)
- **Battery Status**: 85% (percentage placeholder)

## Responsive Layout Features

### Dynamic Text Fitting
- All text uses `shortenTextToFit()` function
- Binary search algorithm for optimal character count
- Adds "..." when text is truncated
- Ensures text never overflows allocated space

### Adaptive Spacing
- Consistent 37px spacing per departure entry
- Reserved disruption space whether used or not
- Maintains uniform appearance across all entries

### Screen Utilization
- **Header**: 5.2% of screen height (25/480px)
- **Content Area**: 94.8% of screen height (455/480px)
- **Margins**: 2.5% of screen width (10px each side)

## Memory Considerations
- **Display Buffer**: Managed by GxEPD2 library
- **Partial Updates**: Supported for individual sections
- **Full Updates**: Required when both weather and departure data change
- **Text Processing**: Dynamic string manipulation for fitting

## Update Performance
- **Full Screen**: Complete redraw (~2-3 seconds)
- **Partial Updates**: Weather or departure section only (~1-2 seconds)
- **Header**: Updated with full screen refreshes only
- **Footer Time**: Updates with departure section refreshes
