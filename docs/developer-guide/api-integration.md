# API Integration

This project requires several APIs. some of API is free to use without API keys
Some of API is free, but need an API Key.
All keys are stored locally and never transmitted to third parties.
If you scale up with large amount of API request, you need a commercial usage aggrement with
API Provider to get large volume request available API Keys.

## Required API Keys

### Overview

- **Google Geolocation API**: Location detection (requires key)
- **RMV Transport API**: German public transport data (requires key)
- **Open-Meteo Weather API**: Weather data via DWD (free, no key)
- **OpenStreetMap Nominatim**: Location names (free, no key)

### 1. Basic Setup for API Keys Encryption

All API Keys must be encrypted in `AES-128-CBC`.
`ENCRYPTION_KEY` is used to encrypt/decrypt the API keys stored in the device.

For detail on how to encrypt the API Keys, see:
https://github.com/gogo-boot/aes-demo

```bash
cp include/secrets/general_secrets.h.example include/secrets/general_secrets.h
```

### 1. Google Geolocation API

**Purpose**: Automatic approximate longitude latitude location detection.

#### Setup Steps:

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing one
3. Enable the **Geolocation API**
4. Create credentials (API Key)
5. Restrict the API key to Geolocation API only (recommended)
6. Encrypt the API Key with Encryption Key

#### Configuration:

```cpp
// include/secrets/general_secrets.h
#pragma once

#define GOOGLE_API_KEY "your_google_api_key_here"
```

#### Cost:

- **Free tier**: 40,000 requests/month
- **Usage**: ~1 request per device setup (one-time)

---

### 2. RMV Transport API

**Purpose**:

1. Getting nearby Stations by geo coordinate information
1. Real-time public transport departure information

#### Setup Steps:

1. Visit [RMV Open Data Portal](https://opendata.rmv.de/)
2. Register for an account
3. Request API access for HAFAS API
4. Obtain your API key
5. Encrypt the API Key with Encryption Key

#### Configuration:

```cpp
// include/secrets/general_secrets.h
#pragma once

#define RMV_API_KEY "your_rmv_api_key_here"
```

#### Coverage:

- **Region**: Hesse, Germany (Frankfurt, Wiesbaden, Kassel, etc.)
- **Transport**: Trains, buses, trams, subways
- **Cost**: Free for non-commercial use

---

### 3. Open-Meteo Weather API

**Purpose**: Weather information using DWD (German Weather Service) data by longitude and latitude

#### Setup:

**No API key required** - Open-Meteo provides free access to DWD weather data.

#### Configuration:

Already configured in `src/api/dwd_weather_api.cpp`:

#### Setup Steps:

- visit https://open-meteo.com/
- read docs and test several online API calls

#### Coverage:

- **Region**: Germany and surrounding European areas
- **Data**: Temperature, humidity, precipitation, weather conditions, forecasts
- **Cost**: Free (up to 10,000 requests/day)
- **Rate Limits**: 10,000 API calls per day, 5,000 per hour

#### Features:

- Real-time weather data from DWD
- Hourly forecasts up to 7 days
- Historical weather data
- No registration required

#### Configurable Weather Models:

Users can select a weather model in the configuration page. The `&models=` parameter is
appended to the Open-Meteo API URL when a specific model is chosen.

| User Label | API Value | Forecast | Resolution | Region |
|---|---|---|---|---|
| Automatisch (Standard) | *(omitted)* | 7 days | varies | Global |
| DWD ICON (Deutschland) | `icon_seamless` | 7 days | 2-11 km | Germany |
| ECMWF (Europa) | `ecmwf_ifs025` | 7 days | 25 km | Europe |
| Meteo-France (Frankreich) | `meteofrance_seamless` | 4 days | 1-25 km | France |
| MeteoSwiss (Schweiz) | `meteoswiss_icon_seamless` | 5 days | 1-2 km | Switzerland |
| ItaliaMeteo (Italien) | `italia_meteo_arpae_icon_2i` | 3 days | 2 km | Italy |

**Note**: Some models provide fewer than 7 days of daily forecast. The display gracefully
handles this by rendering only the available days (remaining space is left empty).

**UV Index**: Only available in Auto mode (no `&models=` parameter). All specific models
return `null` for `uv_index_max`. The display hides the UV section when data is unavailable.

The JSON response format is identical regardless of model selection — only the data
coverage and resolution differ.

#### Day Browsing (RTC-Cached Multi-Day Fetch)

Day browsing (day 1–6 in Weather-Only mode) does **not** make a per-day API call. Instead,
the per-day hourly data is prefetched into RTC memory during the regular weather update, so
a button press renders instantly with no WiFi round-trip.

**How it works:**

When the weather update interval elapses, the device makes **one wide** Open-Meteo request
with `forecast_days=N` and slices the response per day into an RTC cache
(`getWeatherHourlyMultiDay()`):

```
https://api.open-meteo.com/v1/forecast
  ?latitude=50.11&longitude=8.68
  &hourly=temperature_2m,precipitation,weather_code,relative_humidity_2m
  &forecast_days=7
  &timezone=Europe/Berlin
  &models=icon_seamless          ← only if a specific model is configured
```

**Key details:**

- **One call for all days**: the wide fetch replaces the previous per-day approach; it runs
  during the normal weather update while WiFi is already up.
- **RTC cache**: each hourly point is a compact `DayBrowsePoint` (12 bytes); the full cache
  is `DayBrowsePoint dayCache[7][24]` (~2 KB RTC). Hour is derived from the array index (no
  time string stored per point).
- **Graph range**: the day-browse graph shows 06:00–24:00 (19 points; the 24:00 point is the
  next cached day's 00:00). The last browsable day shows 18 points (06:00–23:00).
- **Limited-day models**: Open-Meteo returns all `forecast_days` timestamps even for a 3- or
  5-day model, filling values past the model's range with `null`. A day counts as valid only
  when hours 06:00–23:00 are all non-null; the cache stores the contiguous valid-day count.
- **Instant browsing**: because all days live in RTC, a day-browse button wake renders from
  cache and skips WiFi entirely when no weather update is due (see
  [Boot Process](boot-process.md)).
- **Fallback**: if the cache is empty or the selected day is not cached, the display falls
  back to the normal today weather view.

**Model compatibility:**

Some weather models provide fewer than 7 forecast days. The `availableForecastDays` variable
(set from `weather.dailyForecastCount`) limits the browsable range. If a user selects
ItaliaMeteo (3 days), they can only browse days 1–2.

| Weather Model | Forecast Days | Browsable Range |
|---------------|---------------|-----------------|
| Auto / DWD ICON / ECMWF | 7 | Day 1–6 |
| MeteoSwiss | 5 | Day 1–4 |
| Meteo-France | 4 | Day 1–3 |
| ItaliaMeteo | 3 | Day 1–2 |

---

### 4. OpenStreetMap Nominatim API

**Purpose**: It is used by application configuration

1. Getting City/Town Name from geocoding coordinates while application configuration
1. Getting longitude/latitude from City/Town Name while application configuration

#### Setup:

Visit https://nominatim.org/ and read documentation.
**No API key required** - Nominatim is free and open source.

#### Configuration:

Already configured in the location detection code:

#### Coverage:

- **Global**: Worldwide coverage from OpenStreetMap data
- **Cost**: Free
- **Rate Limits**: 1 request per second (respected by the device)
- **Usage Policy**: Must include proper attribution, reasonable use

#### Important Notes:

- **Fair Use Policy**: Nominatim is provided as a free service
- **Rate Limiting**: Device automatically respects 1 request/second limit
- **Attribution**: OpenStreetMap data contributors are credited
- **Backup Options**: For high-volume applications, consider self-hosting

---

## API Key Security

### Best Practices

1. **Never commit secrets to Git**
    - Use `.gitignore` to exclude `include/secrets/general_secrets.h` (except examples)
    - Check your repository history for accidentally committed keys

2. **Restrict API key permissions**
    - Google: Limit to Geolocation API only
    - RMV: Use for personal/development purposes only

3. **Monitor usage**
    - Check Google Cloud Console for unexpected usage
    - Monitor RMV API quotas

## Other Transport APIs

According to recent reports, RMV and NRW public transport systems have been experiencing high delays and cancellations:
https://www.adac.de/news/test-oepnv-ausfaelle-verspaetungen/
NRW API implementation will be followed up in future releases.

### Germany

- **DB API** - Deutsche Bahn (national railways) : cost near 4000 Euros, to get one API access (State 2025)
- **RMV API** - Rhein-Main transport (default) : 67% S-Bahn on-time rate (less than 3 minutes delay)
- **NRW API** - North Rhine-Westphalia transport : Köln, Düsseldorf, Dortmund
- **VBB API** - Berlin-Brandenburg transport
- **MVV API** - Munich transport
- **HVV API** - Hamburg transport : over 95% high on-time rate in public transport, may not need to implement

### International

- **GTFS** - General Transit Feed Specification (worldwide)
- **TfL API** - Transport for London
- **RATP API** - Paris transport
- **SL API** - Stockholm transport

### Adapting for Other APIs

1. Create new API files in `src/api/`
2. Implement similar functions to `rmv_api.cpp`
3. Update configuration to use new API
4. Add new secrets file if required

## Next Steps

After setting up API keys:

- [Development Setup](development-setup.md) - Set up your development environment
- [Configuration Layers](configuration-layers.md) - Understanding configuration system
- [Run Book](run-book.md) - Operational procedures and commands
