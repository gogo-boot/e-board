# API Keys Setup

This project requires API keys from several external services. All keys are stored locally and never transmitted to third parties.

## Required API Keys

### 1. Google Geolocation API

**Purpose**: Automatic location detection for finding nearby transport stops.

#### Setup Steps:
1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing one
3. Enable the **Geolocation API**
4. Create credentials (API Key)
5. Restrict the API key to Geolocation API only (recommended)

#### Configuration:
```cpp
// src/secrets/google_secrets.h
#pragma once

#define GOOGLE_API_KEY "your_google_api_key_here"
```

#### Cost: 
- **Free tier**: 40,000 requests/month
- **Usage**: ~1 request per device setup (one-time)

---

### 2. RMV Transport API

**Purpose**: Real-time public transport departure information for German regional transport.

#### Setup Steps:
1. Visit [RMV Open Data Portal](https://opendata.rmv.de/)
2. Register for an account
3. Request API access for HAFAS API
4. Obtain your API key

#### Configuration:
```cpp
// src/secrets/rmv_secrets.h
#pragma once

#define RMV_API_KEY "your_rmv_api_key_here"
#define RMV_BASE_URL "https://www.rmv.de/hapi/"
```

#### Coverage:
- **Region**: Hesse, Germany (Frankfurt, Wiesbaden, Kassel, etc.)
- **Transport**: Trains, buses, trams, subways
- **Cost**: Free for non-commercial use

---

### 3. DWD Weather API

**Purpose**: Weather information from German Weather Service.

#### Setup:
**No API key required** - DWD provides open weather data.

#### Configuration:
Already configured in `src/api/dwd_weather_api.cpp`:
```cpp
// No secrets file needed
#define DWD_BASE_URL "https://api.brightsky.dev/weather"
```

#### Coverage:
- **Region**: Germany and surrounding areas
- **Data**: Temperature, humidity, precipitation, weather conditions
- **Cost**: Free

---

## Setting Up Secret Files

### 1. Copy Example Files
```bash
cd src/secrets/
cp google_secrets.h.example google_secrets.h
cp rmv_secrets.h.example rmv_secrets.h
```

### 2. Edit Configuration Files

#### Google Secrets
```cpp
// src/secrets/google_secrets.h
#pragma once

// Get your API key from: https://console.cloud.google.com/
#define GOOGLE_API_KEY "your_google_api_key_here"

// Optional: Restrict to specific domains/IPs
#define GOOGLE_API_REFERRER "your-domain.com"
```

#### RMV Secrets
```cpp
// src/secrets/rmv_secrets.h
#pragma once

// Get your API key from: https://opendata.rmv.de/
#define RMV_API_KEY "your-rmv-api-key-here"

// RMV API endpoint (usually doesn't change)
#define RMV_BASE_URL "https://www.rmv.de/hapi/"

// Optional: Default location for testing
#define RMV_DEFAULT_STOP_ID "3006907"  // Frankfurt Hauptbahnhof
```

## API Key Security

### Best Practices
1. **Never commit secrets to Git**
   - Use `.gitignore` to exclude `src/secrets/*.h` (except examples)
   - Check your repository history for accidentally committed keys

2. **Restrict API key permissions**
   - Google: Limit to Geolocation API only
   - RMV: Use for personal/development purposes only

3. **Monitor usage**
   - Check Google Cloud Console for unexpected usage
   - Monitor RMV API quotas

### Git Security Check
```bash
# Ensure secrets are not tracked
git status src/secrets/

# Should only show example files, not actual secret files
# If you see google_secrets.h or rmv_secrets.h, add them to .gitignore
```

## Testing API Keys

### Manual Testing
Use the serial monitor to verify API responses:

1. **Flash firmware** with your API keys
2. **Monitor serial output** during startup
3. **Look for successful API calls**:
   ```
   [GOOGLE] Location detected: 50.1109, 8.6821
   [RMV] Found 15 nearby stops
   [DWD] Weather data retrieved successfully
   ```

### Error Messages
Common error patterns:

#### Google API Errors
```
[ERROR] Google API: 403 Forbidden
→ Check API key validity and permissions

[ERROR] Google API: 429 Too Many Requests  
→ Rate limit exceeded, wait and retry
```

#### RMV API Errors
```
[ERROR] RMV API: Invalid API key
→ Check RMV_API_KEY in secrets file

[ERROR] RMV API: No stops found
→ Location might be outside RMV coverage area
```

## Alternative Transport APIs

If you're outside the RMV coverage area, you can adapt the code for other transport APIs:

### Germany
- **DB API** - Deutsche Bahn (national railways)
- **VBB API** - Berlin-Brandenburg transport
- **MVV API** - Munich transport
- **HVV API** - Hamburg transport

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

## Troubleshooting

### Common Issues

#### "API key not found" errors
- Verify secrets files exist and contain valid keys
- Check for typos in `#define` statements
- Ensure files are saved with correct encoding (UTF-8)

#### Location detection fails
- Check Google API key has Geolocation API enabled
- Verify internet connectivity
- Test with different WiFi networks

#### No transport data
- Confirm you're in RMV coverage area (Hesse, Germany)
- Check RMV API key validity
- Try with known stop IDs for testing

### Debug Mode
Enable verbose logging to troubleshoot API issues:
```cpp
esp_log_level_set("GOOGLE", ESP_LOG_DEBUG);
esp_log_level_set("RMV", ESP_LOG_DEBUG);
esp_log_level_set("DWD", ESP_LOG_DEBUG);
```

## Next Steps
After setting up API keys:
- [Quick Start Guide](./quick-start.md) - Initial device configuration
- [Configuration Guide](./configuration.md) - Detailed setup options
