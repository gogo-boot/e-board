# Configuration Process Flowchart

## 🔄 System Boot Decision Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            SYSTEM BOOT                                  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         hasConfigInNVS()                                │
│                                                                         │
│  1. ConfigManager::loadConfig(g_stationConfig)                          │
│  2. Validate critical fields:                                           │
│     • selectedStopId.length() > 0                                       │
│     • ssid.length() > 0                                                 │
│     • latitude != 0.0                                                   │
│     • longitude != 0.0                                                  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌─────────────────────────────────┐
                    │     Valid Configuration?        │
                    └─────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                   YES                             NO
                    │                               │
                    ▼                               ▼
┌─────────────────────────────────┐   ┌─────────────────────────────────┐
│       runOperationalMode()      │   │      runConfigurationMode()     │
│                                 │   │                                 │
│ • inConfigMode = false          │   │ • inConfigMode = true           │
│ • saveConfigMode(false)         │   │ • saveConfigMode(true)          │
│ • Load from NVS + RTC           │   │ • Setup WiFi AP mode            │
│ • Connect to WiFi               │   │ • Start web server              │
│ • Fetch data                    │   │ • Get location & stops          │
│ • Enter deep sleep              │   │ • Handle web requests           │
└─────────────────────────────────┘   └─────────────────────────────────┘
                    │                               │
                    ▼                               ▼
┌─────────────────────────────────┐   ┌─────────────────────────────────┐
│         DEEP SLEEP              │   │        STAY AWAKE               │
│                                 │   │                                 │
│ • Wake every N minutes          │   │ • Handle web requests           │
│ • Use RTC fast path             │   │ • Wait for user config          │
│ • Update display                │   │ • Process form submissions      │
│ • Return to sleep               │   │ • Save to NVS + RTC             │
└─────────────────────────────────┘   └─────────────────────────────────┘
```

## 💾 Configuration Save Process

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        USER SAVES CONFIGURATION                         │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         Web Form Submission                             │
│                                                                         │
│  1. User fills form and clicks "Save"                                  │
│  2. JavaScript collects all form values                                │
│  3. POST /save_config with JSON payload                                │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      config_page.cpp Processing                         │
│                                                                         │
│  1. Parse JSON from request body                                        │
│  2. Update g_stationConfig struct with new values                       │
│  3. Call ConfigManager::saveConfig(g_stationConfig)                     │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       ConfigManager::saveConfig()                       │
│                                                                         │
│  Step 1: Save to NVS (Complete Data)                                    │
│  ─────────────────────────────────────────                              │
│  • preferences.begin("mystation", false)                                │
│  • preferences.putFloat("lat", config.latitude)                         │
│  • preferences.putFloat("lon", config.longitude)                        │
│  • preferences.putString("city", config.cityName)                       │
│  • preferences.putString("stopId", config.selectedStopId)               │
│  • preferences.putString("ssid", config.ssid)                           │
│  • ... (all other configuration fields)                                 │
│  • preferences.end()                                                    │
│                                                                         │
│  Step 2: Save to RTC Memory (Critical Data Only)                        │
│  ─────────────────────────────────────────────────                      │
│  • saveCriticalToRTC(config)                                            │
│    - rtcConfig.isValid = true                                           │
│    - rtcConfig.latitude = config.latitude                               │
│    - rtcConfig.longitude = config.longitude                             │
│    - rtcConfig.selectedStopId = config.selectedStopId                   │
│    - rtcConfig.selectedStopName = config.selectedStopName               │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           Configuration Saved                           │
│                                                                         │
│  • NVS: Complete config (survives power loss)                           │
│  • RTC: Critical data (survives deep sleep)                             │
│  • Switch to operational mode                                           │
│  • Enter deep sleep                                                     │
└─────────────────────────────────────────────────────────────────────────┘
```

## 📥 Configuration Load Process

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    OPERATIONAL MODE BOOT/WAKE                           │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                        Load Configuration                               │
│                                                                         │
│  1. ConfigManager::loadConfig(g_stationConfig)                          │
│     • Load complete configuration from NVS                              │
│     • ~50-100ms operation (slow but complete)                           │
│                                                                         │
│  2. Check if deep sleep wake-up:                                        │
│     • !configMgr.isFirstBoot()                                          │
│     • configMgr.loadCriticalFromRTC(g_stationConfig)                    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌─────────────────────────────────┐
                    │      RTC Data Available?        │
                    └─────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                   YES                             NO
                    │                               │
                    ▼                               ▼
┌─────────────────────────────────┐   ┌─────────────────────────────────┐
│        FAST PATH (RTC)          │   │        SLOW PATH (NVS)          │
│                                 │   │                                 │
│ • Use RTC critical data         │   │ • Use NVS complete data         │
│ • ~1ms load time                │   │ • ~50-100ms load time           │
│ • Override critical fields:     │   │ • All configuration fields      │
│   - latitude                    │   │ • First boot or RTC invalid     │
│   - longitude                   │   │                                 │
│   - selectedStopId              │   │                                 │
│   - selectedStopName            │   │                                 │
│ • Keep other fields from NVS    │   │                                 │
└─────────────────────────────────┘   └─────────────────────────────────┘
                    │                               │
                    └───────────────┬───────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           Continue Operation                            │
│                                                                         │
│  • Connect to WiFi using config.ssid                                    │
│  • Fetch departure data using config.selectedStopId                     │
│  • Get weather using config.latitude, config.longitude                  │
│  • Update display                                                       │
│  • Enter deep sleep for config.transportInterval minutes                │
└─────────────────────────────────────────────────────────────────────────┘
```

## 🏃‍♂️ Performance Comparison

| Operation | RTC Memory | NVS Storage | Use Case |
|-----------|------------|-------------|----------|
| **Read Time** | ~1ms | ~50-100ms | Fast wake-up |
| **Write Time** | ~1ms | ~10-50ms | Quick save |
| **Capacity** | 8KB total | Large | Critical vs Complete |
| **Persistence** | Deep sleep only | Power loss | Temporary vs Permanent |
| **Access Pattern** | Every wake | First boot/config | Frequent vs Rare |

## 🔄 Mode Transitions

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          MODE TRANSITIONS                               │
└─────────────────────────────────────────────────────────────────────────┘

Configuration Mode ──────────────────────────────────────► Operational Mode
                                                                     │
      ▲                                                              │
      │                                                              │
      │            ┌─────────────────────────────────────────────────┘
      │            │
      │            ▼
      │
      │        Deep Sleep ◄──────────────────────────────────────────┘
      │            │
      │            │ (Wake every N minutes)
      │            │
      │            ▼
      │
      └────── RTC Fast Path ──────────────────────────────────────────┘
                   │
                   │ (Use saved critical data)
                   │
                   ▼
                   
              Continue Operation
```

## 🎯 Key Decision Points

1. **Boot Decision**: `hasConfigInNVS()` - Valid config → Operational, Invalid → Configuration
2. **Load Strategy**: `isFirstBoot()` - First boot → NVS only, Wake-up → RTC + NVS
3. **Save Strategy**: Always save to both NVS (complete) and RTC (critical)
4. **Mode Management**: `configMode` flag persists across power cycles

This flowchart provides a visual overview of the configuration process, making it easy to understand the system behavior and debug issues.
