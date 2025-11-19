# Software Setup

## Development Environment

### Prerequisites
- **PlatformIO IDE** (VS Code extension) or PlatformIO Core
- **Git** for version control
- **ESP32-C3 USB drivers** (usually automatic)

### Installation Steps

#### 1. Install PlatformIO
```bash
# Option A: VS Code Extension (Recommended)
# Install "PlatformIO IDE" extension in VS Code

# Option B: Command Line
pip install platformio
```

#### 2. Clone Repository
```bash
git clone <your-repository-url>
cd e-board
```

#### 3. Install Dependencies
```bash
# PlatformIO will automatically install dependencies from platformio.ini
pio lib install
```

## Project Structure

```
e-board/
├── platformio.ini          # PlatformIO configuration
├── README.md               # Quick overview
├── docs/                   # Detailed documentation
├── data/                   # Web files (HTML, CSS, JS)
│   └── config_my_station.html
├── src/                    # Source code
│   ├── main.cpp           # Main application
│   ├── api/               # External API integrations
│   │   ├── dwd_weather_api.cpp
│   │   ├── google_api.cpp
│   │   └── rmv_api.cpp
│   ├── config/            # Configuration management
│   │   ├── config_page.cpp
│   │   ├── config_struct.h
│   │   └── pins.h
│   ├── secrets/           # API keys (not in git)
│   │   ├── google_secrets.h.example
│   │   └── rmv_secrets.h.example
│   └── util/              # Utility functions
│       ├── power.h
│       ├── sleep_utils.cpp
│       ├── util.cpp
│       └── weather_print.cpp
└── test/                  # Test data and scripts
```

## Configuration Files

### 1. PlatformIO Configuration
The `platformio.ini` file contains build settings:

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_deps = 
    tzapu/WiFiManager@^0.16.0
    bblanchon/ArduinoJson@^7.0.4
    # Add other dependencies as needed
```

### 2. API Keys Setup
Copy example files and add your API keys:

```bash
# Copy example files
cp src/secrets/google_secrets.h.example src/secrets/google_secrets.h
cp src/secrets/rmv_secrets.h.example src/secrets/rmv_secrets.h

# Edit with your API keys
# See: docs/api-keys.md for obtaining keys
```

## Building and Flashing

### 1. Build Project
```bash
# Build firmware
pio run

# Build filesystem (for web files)
pio run --target buildfs
```

### 2. Upload Firmware
```bash
# Upload firmware
pio run --target upload

# Upload filesystem
pio run --target uploadfs

# Upload both
pio run --target upload && pio run --target uploadfs
```

### 3. Monitor Serial Output
```bash
# Open serial monitor
pio device monitor

# Or with specific baud rate
pio device monitor --baud 115200
```

## IDE Configuration

### VS Code Settings
Recommended VS Code settings for this project:

```json
{
    "files.associations": {
        "*.ino": "cpp",
        "*.h": "c"
    },
    "C_Cpp.intelliSenseEngine": "Tag Parser",
    "platformio-ide.activateOnlyOnPlatformIOProject": true
}
```

### Extensions
Recommended VS Code extensions:
- **PlatformIO IDE** - Essential for ESP32 development
- **C/C++** - Code completion and IntelliSense
- **GitLens** - Enhanced Git capabilities
- **Bracket Pair Colorizer** - Code readability

## Compilation Flags

### Debug vs Release
```ini
# Debug build (default)
build_flags = -DDEBUG_ESP_PORT=Serial -DDEBUG_ESP_CORE

# Release build (optimized)
build_flags = -O2 -DNDEBUG
```

### Custom Defines
```ini
build_flags = 
    -DCONFIG_ARDUINOJSON_DECODE_NESTING_LIMIT=200
    -DWIFI_SSID_MAX_LENGTH=32
    -DESP_LOG_LEVEL=ESP_LOG_DEBUG
```

## Filesystem (LittleFS)

### File Structure
The `data/` directory contains files uploaded to the ESP32's filesystem:

```
data/
└── config_my_station.html  # Web configuration interface
```

### Upload Web Files
```bash
# Upload filesystem
pio run --target uploadfs

# Or use PlatformIO GUI
# Tasks → esp32-c3-devkitm-1 → Platform → Upload Filesystem Image
```

## Serial Debugging

### Log Levels
```cpp
esp_log_level_set("*", ESP_LOG_DEBUG);  // All modules
esp_log_level_set("WIFI", ESP_LOG_INFO); // Specific module
```

### Available Log Levels
- `ESP_LOG_ERROR` - Critical errors only
- `ESP_LOG_WARN` - Warnings and errors
- `ESP_LOG_INFO` - General information
- `ESP_LOG_DEBUG` - Detailed debugging
- `ESP_LOG_VERBOSE` - Maximum detail

## Common Build Issues

### Missing Dependencies
```bash
# Clean and rebuild
pio run --target clean
pio lib install
pio run
```

### Board Not Detected
```bash
# List available devices
pio device list

# Manual port specification
pio run --target upload --upload-port /dev/ttyUSB0
```

### Filesystem Upload Fails
```bash
# Erase flash completely
esptool.py --chip esp32c3 erase_flash

# Re-upload firmware and filesystem
pio run --target upload
pio run --target uploadfs
```

## Version Control

### Git Ignore
Ensure your `.gitignore` includes:
```gitignore
# Secrets
src/secrets/google_secrets.h
src/secrets/rmv_secrets.h

# Build artifacts
.pio/
.vscode/
*.bin
*.elf

# OS files
.DS_Store
Thumbs.db
```

## Next Steps
After software setup:
- [API Keys Setup](./api-keys.md) - Configure external services
- [Quick Start Guide](./quick-start.md) - First run configuration
- [Development Guide](./development.md) - Contributing to the project
