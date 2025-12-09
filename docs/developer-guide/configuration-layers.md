# Configuration System Architecture

## Why Multiple Configuration Layers?

MyStation's configuration system spans multiple layers of the application stack, each serving a specific purpose.
Understanding why this complexity exists is crucial for maintaining and extending the system.

## The Five-Layer Architecture

```mermaid
graph TD
    A[User Interface Layer] -->|User Input| B[Web/JavaScript Layer]
    B -->|HTTP POST JSON| C[C++ Application Layer]
    C -->|Serialize| D[NVS Storage Layer]
    D -->|Persist| E[Flash Memory]
    E -->|Boot/Wake| D
    D -->|Deserialize| C
    C -->|Template Variables| A
    style A fill: #e1f5ff
    style B fill: #fff4e1
    style C fill: #ffe1f5
    style D fill: #e1ffe1
    style E fill: #f0f0f0
```

### Layer 1: HTML/UI Layer (User Interface)

**Purpose**: Present configuration options to the user

**Technology**: HTML form elements with IDs

**Example**:

```html
<input type="number" id="weather-interval" value="{{WEATHER_INTERVAL}}">
<input type="text" id="city-display" value="{{CITY_NAME}}">
```

**Why This Layer?**

- **User-Friendly**: Provides human-readable labels and input controls
- **Validation**: Browser-native input validation (min/max, required, etc.)
- **Accessibility**: Standard HTML forms work with screen readers and assistive devices
- **Template System**: Mustache-style `{{VARIABLES}}` allow dynamic content injection

**Naming Convention**: `kebab-case` (e.g., `weather-interval`, `city-display`)

---

### Layer 2: JavaScript/JSON Layer (Data Transport)

**Purpose**: Collect form data and transmit to server

**Technology**: JavaScript objects and JSON serialization

**Example**:

```javascript
var weatherInterval = document.getElementById('weather-interval').value;
var config = {
    weatherInterval: parseInt(weatherInterval),
    cityName: document.getElementById('city-display').value
};

fetch('/save-config', {
    method: 'POST',
    body: JSON.stringify(config)
});
```

**Why This Layer?**

- **Type Conversion**: Convert string inputs to proper types (int, float, bool)
- **Validation**: Client-side validation before sending to server
- **Structured Data**: JSON provides clear, type-safe data structure
- **Debugging**: Easy to inspect network traffic and payloads
- **Standard Protocol**: HTTP POST with JSON is universal and well-understood

**Naming Convention**: `camelCase` (e.g., `weatherInterval`, `cityName`)

---

### Layer 3: C++ Application Layer (Business Logic)

**Purpose**: Process configuration, apply defaults, validate ranges

**Technology**: C++ struct with member variables

**Example**:

```cpp
struct StationConfig {
    int weatherInterval = 60;      // Default: 60 minutes
    char cityName[32] = "Frankfurt";  // Default city
    float latitude = 50.1109;
    float longitude = 8.6821;

    // ... other fields
};
```

**Why This Layer?**

- **Type Safety**: Strong typing prevents errors
- **Default Values**: Compile-time defaults ensure system always has valid config
- **Memory Management**: Fixed-size arrays prevent overflow
- **Performance**: Direct memory access, no parsing overhead
- **Validation**: Range checks and business logic validation
- **Cross-Module Access**: Other C++ modules can directly access configuration

**Naming Convention**: `camelCase` (e.g., `weatherInterval`, `cityName`)

---

### Layer 4: NVS Storage Layer (Persistence)

**Purpose**: Store configuration in non-volatile flash memory

**Technology**: ESP32 NVS (Non-Volatile Storage) API

**Example**:

```cpp
// Save
preferences.putInt("weatherInt", config.weatherInterval);
preferences.putString("city", config.cityName);

// Load
config.weatherInterval = preferences.getInt("weatherInt", 60);
preferences.getString("city", config.cityName, sizeof(config.cityName));
```

**Why This Layer?**

- **Persistence**: Survives power loss, reboots, firmware updates
- **Key-Value Store**: Simple, efficient storage mechanism
- **Atomic Operations**: NVS handles flash write cycles and wear leveling
- **Separate Namespace**: Can isolate WiFi credentials in encrypted namespace
- **Version Migration**: Can detect and migrate old configuration formats

**Naming Convention**: Abbreviated (≤15 chars) due to NVS limitations

- Examples: `weatherInt`, `city`, `stopId`, `transStart`

**Why Abbreviated?**

- **NVS Limitation**: Keys limited to 15 characters maximum
- **Flash Efficiency**: Shorter keys = less flash usage
- **Performance**: Faster key lookups with shorter strings

---

### Layer 5: Flash Memory (Hardware)

**Purpose**: Physical storage of configuration data

**Technology**: SPI Flash with wear leveling

**Why This Layer?**

- **Non-Volatile**: Data persists when power is removed
- **Wear Leveling**: NVS library manages flash cell wear automatically
- **Partition Isolation**: Configuration stored in dedicated NVS partition
- **Backup**: Can implement backup/restore mechanisms

---

## Data Flow: User to Flash

### Save Configuration Flow

```mermaid
sequenceDiagram
    participant User
    participant HTML as HTML Form
    participant JS as JavaScript
    participant HTTP as Web Server
    participant CPP as C++ Config Manager
    participant NVS as NVS Storage
    participant Flash as Flash Memory
    User ->> HTML: Fill form & click Save
    HTML ->> JS: Trigger submit event
    JS ->> JS: Collect form values
    JS ->> JS: Convert types (string→int)
    JS ->> JS: Create JSON object
    JS ->> HTTP: POST /save-config
    HTTP ->> CPP: Parse JSON
    CPP ->> CPP: Validate values
    CPP ->> CPP: Update config struct
    CPP ->> NVS: putInt("weatherInt", value)
    CPP ->> NVS: putString("city", value)
    NVS ->> Flash: Write to partition
    Flash -->> NVS: Write confirmed
    NVS -->> CPP: Save successful
    CPP -->> HTTP: 200 OK
    HTTP -->> JS: Success response
    JS -->> User: Show "Saved!" message
```

### Load Configuration Flow

```mermaid
sequenceDiagram
    participant Boot as System Boot
    participant CPP as C++ Config Manager
    participant NVS as NVS Storage
    participant Flash as Flash Memory
    participant Web as Web Server
    participant User
    Boot ->> CPP: loadConfig()
    CPP ->> NVS: getInt("weatherInt", default)
    NVS ->> Flash: Read from partition
    Flash -->> NVS: Return value
    NVS -->> CPP: Return value or default
    CPP ->> CPP: Populate config struct
    User ->> Web: GET /config
    Web ->> CPP: Get config values
    CPP -->> Web: Return config struct
    Web ->> Web: Replace {{TEMPLATES}}
    Web -->> User: Render HTML page
```

---

## Why Not a Single Layer?

### ❌ Anti-Pattern: Direct HTML to NVS

**What if we skipped the intermediate layers?**

```html
<!-- BAD: HTML directly to NVS -->
<input type="number" nvs-key="weatherInt" value="60">
<script>
    // Directly save to NVS via some API
    NVS.put(element.getAttribute('nvs-key'), element.value);
</script>
```

**Problems**:

1. **No Type Safety**: Everything is a string
2. **No Validation**: Can save invalid values
3. **No Defaults**: What if NVS is empty?
4. **No Business Logic**: Can't apply complex rules
5. **Tight Coupling**: HTML tied to NVS key names
6. **Hard to Test**: Can't unit test without hardware
7. **No Migration**: Can't change storage format

---

## Key Naming Mapping Strategy

### The Transformation Path

```mermaid
graph LR
    A[User Input<br/>Weather Update Interval] --> B[HTML ID<br/>weather-interval]
    B --> C[JavaScript Variable<br/>weatherInterval]
    C --> D[JSON Key<br/>weatherInterval]
    D --> E[C++ Member<br/>weatherInterval]
    E --> F[NVS Key<br/>weatherInt]
    style A fill: #e1f5ff
    style B fill: #fff4e1
    style C fill: #ffe1f5
    style D fill: #ffe1f5
    style E fill: #e1ffe1
    style F fill: #f0f0f0
```

### Naming Rules

| Layer      | Convention  | Reason                   | Example            |
|------------|-------------|--------------------------|--------------------|
| HTML       | kebab-case  | HTML/CSS standard        | `weather-interval` |
| JavaScript | camelCase   | JavaScript convention    | `weatherInterval`  |
| JSON       | camelCase   | JSON/REST API standard   | `weatherInterval`  |
| C++        | camelCase   | C++ convention (members) | `weatherInterval`  |
| NVS        | Abbreviated | 15-char limit            | `weatherInt`       |

### Consistency Benefits

- **Predictable**: Developers can guess key names
- **Maintainable**: Easy to trace through layers
- **Debuggable**: Can search for variations of same name
- **Documented**: Clear mapping tables exist

---

## Special Cases

### 1. Arrays and Lists (Transport Filters)

**Problem**: NVS doesn't support arrays directly

**Solution**: Store count + individual items

```cpp
// Save array
preferences.putInt("filterCount", filterCount);
for (int i = 0; i < filterCount; i++) {
    String key = "filter" + String(i);
    preferences.putString(key.c_str(), filters[i].c_str());
}

// Load array
int count = preferences.getInt("filterCount", 0);
for (int i = 0; i < count && i < MAX_FILTERS; i++) {
    String key = "filter" + String(i);
    preferences.getString(key.c_str(), buffer, sizeof(buffer));
    filters[i] = String(buffer);
}
```

**Why**: NVS is key-value, not a database

---

### 2. Encrypted WiFi Credentials

**Problem**: WiFi passwords should be encrypted

**Solution**: Separate NVS namespace with encryption

```cpp
// Regular config
Preferences preferences;
preferences.begin("config", false);  // Not encrypted

// WiFi credentials
Preferences wifiPrefs;
wifiPrefs.begin("wifi", false);  // Could enable encryption
wifiPrefs.putString("ssid", ssid);
wifiPrefs.putString("password", password);
```

**Why**: Security best practice, compliance

---

### 3. RTC Memory (Ultra-Fast Access)

**Problem**: Some data needed immediately after deep sleep wake

**Solution**: Store critical data in RTC RAM

```cpp
RTC_DATA_ATTR int wakeupCount = 0;
RTC_DATA_ATTR DisplayMode temporaryMode = DISPLAY_MODE_NONE;
```

**Why**:

- RTC RAM survives deep sleep
- Faster access than NVS
- No flash wear from frequent writes

**Trade-off**: Lost on full power loss (not just deep sleep)

---

## Configuration Migration

### Version Evolution

```mermaid
graph TD
    A[Boot] --> B{Check Config Version}
    B -->|v1 . 0| C[Load v1.0 Format]
    B -->|v2 . 0| D[Load v2.0 Format]
    B -->|Unknown| E[Use Defaults]
    C --> F[Migrate to v2.0]
    F --> G[Save v2.0 Format]
    D --> G
    E --> G
    G --> H[Normal Operation]
```

**Example Migration**:

```cpp
void migrateConfig() {
    int version = preferences.getInt("cfgVersion", 1);

    if (version == 1) {
        // v1 used "wInt" key, v2 uses "weatherInt"
        int oldValue = preferences.getInt("wInt", 60);
        preferences.putInt("weatherInt", oldValue);
        preferences.remove("wInt");

        preferences.putInt("cfgVersion", 2);
    }
}
```

---

## Best Practices

### Adding New Configuration Fields

**Checklist**:

1. ✅ Add to C++ struct with default value
2. ✅ Add NVS save/load with abbreviated key
3. ✅ Add HTML form element with kebab-case ID
4. ✅ Add template replacement in config_page.cpp
5. ✅ Add JavaScript variable in camelCase
6. ✅ Add to JSON save handler
7. ✅ Add validation (min/max, regex, etc.)
8. ✅ Update documentation
9. ✅ Consider migration from old config

### Validation Strategy

**Defense in Depth**: Validate at multiple layers

```cpp
// Layer 1: HTML (basic)
<input type="number" min="1" max="60" required>

// Layer 2: JavaScript (UX)
if (weatherInterval < 1 || weatherInterval > 60) {
    alert("Weather interval must be 1-60 minutes");
    return;
}

// Layer 3: C++ (security/safety)
if (config.weatherInterval < 1 || config.weatherInterval > 60) {
    config.weatherInterval = 60;  // Force safe default
    ESP_LOGW(TAG, "Invalid weather interval, using default");
}
```

**Why**: Each layer can fail or be bypassed

---

## Performance Considerations

### NVS Access Patterns

**Fast**:

- ✅ Reading config at boot (once)
- ✅ Bulk save operation (all fields at once)

**Slow**:

- ❌ Reading every wake cycle
- ❌ Saving single field repeatedly

**Optimization**: Load once to RAM, save in batches

```cpp
// GOOD: Load once
void setup() {
    ConfigManager::loadConfig();  // Loads everything once
    // ... use g_stationConfig throughout
}

// BAD: Load every time
void loop() {
    int interval = preferences.getInt("weatherInt", 60);  // Don't do this!
}
```

### Memory Usage

| Layer      | Size       | Location |
|------------|------------|----------|
| NVS Keys   | ~300 bytes | Flash    |
| NVS Values | ~500 bytes | Flash    |
| C++ Struct | ~800 bytes | RAM      |
| RTC Data   | ~100 bytes | RTC RAM  |

**Total**: ~1.7 KB (negligible on ESP32)

---

## Debugging Configuration Issues

### Layer-by-Layer Debugging

```mermaid
graph TD
    A[Config Not Saving?] --> B{Check JavaScript Console}
    B -->|Error| C[Fix JS validation]
    B -->|OK| D{Check Network Tab}
    D -->|404/500| E[Fix server endpoint]
    D -->|200 OK| F{Check Serial Monitor}
    F -->|JSON Parse Error| G[Fix JSON format]
    F -->|NVS Error| H[Check NVS partition]
    F -->|Success| I{Check NVS Directly}
    I -->|Value Missing| J[Fix save logic]
    I -->|Value Wrong| K[Fix conversion]
    I -->|Value Correct| L[Check load logic]
```

### Tools

**JavaScript**:

```javascript
console.log('Config to save:', config);  // Before POST
```

**C++ Serial**:

```cpp
ESP_LOGI(TAG, "Received JSON: %s", jsonString.c_str());
ESP_LOGI(TAG, "Saved weatherInt: %d", config.weatherInterval);
```

**NVS Direct Inspection**:

```cpp
void dumpNVS() {
    preferences.begin("config", true);  // Read-only
    int value = preferences.getInt("weatherInt", -1);
    ESP_LOGI(TAG, "NVS weatherInt = %d", value);
    preferences.end();
}
```

---

## Summary

### Why This Complexity?

The five-layer architecture exists because:

1. **Separation of Concerns**: Each layer has one job
2. **Type Safety**: Strong typing prevents errors
3. **Validation**: Multiple validation points
4. **Persistence**: Survives power loss
5. **Performance**: Optimized for embedded systems
6. **Maintainability**: Clear boundaries, easy to debug
7. **Extensibility**: Easy to add new fields
8. **Migration**: Can evolve configuration format

### Key Principle

> **Data flows through transformations, each layer adding value**

- HTML: User interaction
- JavaScript: Client-side logic
- JSON: Transport format
- C++: Business logic and validation
- NVS: Persistence
- Flash: Physical storage

This architecture has served MyStation well and should be maintained as the project evolves.

---

## Related Documentation

- [Configuration Keys Reference](../../reference/configuration-keys-quick-reference.md)
- [Configuration Data Flow](../configuration-data-flow.md)
- [Boot Process](boot-process.md)
- [Development Setup](development-setup.md)

