# Developer Guide

Welcome to the MyStation developer documentation. This guide provides technical details about the system architecture,
implementation, and development processes.

## For Developers

This section is for developers who want to:

- 🔧 Understand the system architecture
- 📝 Contribute to the project
- 🐛 Debug and fix issues
- ✨ Add new features
- 🔌 Modify hardware configuration

> 👤 **Are you a user?** See the [User Guide](../user-guide/index.md) instead.

## Project Overview

MyStation is an ESP32-based e-paper display system that shows real-time German public transport departures and weather
information.

### Key Technologies

- **Platform**: ESP32-C3 / ESP32-S3 (Espressif SoC)
- **Framework**: Arduino framework via PlatformIO
- **Display**: 7.5" e-paper (GDEY075T7, 800x480)
- **Network**: WiFi 2.4 GHz (802.11 b/g/n)
- **Storage**: NVS (Non-Volatile Storage) for configuration
- **OTA**: Over-the-air firmware updates
- **Power**: Deep sleep with intelligent wake-up

### Core Features

1. **Configuration System** - Multi-phase boot with web-based setup
2. **WiFi Management** - Auto-connect, fallback, mDNS
3. **Display System** - Multiple modes, efficient e-paper rendering
4. **Power Management** - Deep sleep, battery monitoring, scheduled wake
5. **OTA Updates** - Secure firmware updates over WiFi
6. **API Integration** - Google Geolocation, RMV Transit, DWD Weather

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   User Interface                    │
│  Web Config Page (SPIFFS) │ E-Paper Display         │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
┌──────────────┴──────────────────────┴───────────────┐
│              Application Layer                      │
│  BootFlowManager │ OTAManager │ ButtonManager       │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
┌──────────────┴──────────────────────┴───────────────┐
│              Service Layer                          │
│  ConfigManager │ WiFiManager │ DisplayManager       │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
┌──────────────┴──────────────────────┴───────────────┐
│              Data Layer                             │
│  RMV API │ DWD API │ Google API │ NVS Storage        │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
┌──────────────┴──────────────────────┴───────────────┐
│              Hardware Layer                         │
│  ESP32 │ SPI │ GPIO │ WiFi Radio │ Deep Sleep        │
└─────────────────────────────────────────────────────┘
```

## Quick Navigation

### 🏗️ Architecture & Design

- [**System Architecture**](architecture.md) - High-level system design
- [**Boot Process**](boot-process.md) - Detailed boot flow and phases
- [**Project Structure**](project-structure.md) - Directory organization and modules
- [**Data Flow**](data-flow.md) - How data moves through the system

### ⚙️ Core Systems

- [**Configuration System**](configuration-system.md) - NVS storage, phases, data structures
- [**Display System**](display-system.md) - E-paper rendering, modes, layouts
- [**Power Management**](power-management.md) - Deep sleep, wake sources, battery
- [**OTA System**](ota-system.md) - Firmware updates, scheduling, rollback

### 📡 Integration & APIs

- [**API Integration**](api-integration.md) - Google, RMV, DWD APIs
- [**WiFi Management**](wifi-management.md) - Connection, fallback, mDNS

### 🛠️ Development

- [**Development Setup**](development-setup.md) - Getting started with PlatformIO
- [**Testing**](testing.md) - Native tests, mocks, debugging
- [**Code Conventions**](code-conventions.md) - Style guide and patterns

### 🔌 Hardware

- [**Pin Configuration**](pin-configuration.md) - GPIO mappings for ESP32-C3/S3
- [**Memory Partitions**](memory-partitions.md) - Flash layout and usage
- [**Hardware Specifications**](hardware-specs.md) - Board comparison and specs

## Technology Stack

### Hardware

| Component | Model         | Purpose                 |
|-----------|---------------|-------------------------|
| MCU       | ESP32-C3 / S3 | Main processor          |
| Display   | GDEY075T7     | 7.5" e-paper, 800x480   |
| WiFi      | Built-in      | 2.4 GHz 802.11 b/g/n    |
| Storage   | 4MB Flash     | Firmware + SPIFFS + NVS |
| Power     | 3.7V LiPo     | Battery powered         |

### Software

| Library       | Version | Purpose            |
|---------------|---------|--------------------|
| PlatformIO    | Latest  | Build system       |
| Arduino-ESP32 | 6.5.0   | Framework          |
| WiFiManager   | 2.0.17  | WiFi configuration |
| ArduinoJson   | 6.21.4  | JSON parsing       |
| GxEPD2        | 1.6.4   | E-paper driver     |
| U8g2          | 1.8.0   | Font rendering     |

### APIs

| Service            | Purpose               | Authentication      |
|--------------------|-----------------------|---------------------|
| Google Geolocation | Location detection    | API Key             |
| RMV API            | Public transport data | API Key             |
| DWD Weather        | Weather forecasts     | None (open)         |
| GitHub             | OTA updates           | Certificate pinning |

## Development Workflow

### 1. Setup Development Environment

```bash
# Install PlatformIO
# See: https://platformio.org/install

# Clone repository
git clone <repository-url>
cd mystation

# Install dependencies (automatic with PlatformIO)
pio run
```

### 2. Configure API Keys

```cpp
// include/secrets/secrets.h
#define GOOGLE_GEOLOCATION_API_KEY "your-key-here"
#define RMV_API_KEY "your-key-here"
```

### 3. Build and Upload

```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Upload filesystem
pio run --target uploadfs

# Monitor serial output
pio device monitor
```

### 4. Development Cycle

1. **Make changes** to source code
2. **Build** and check for errors
3. **Upload** to device
4. **Test** functionality
5. **Monitor** serial output for debugging
6. **Iterate**

## Key Concepts

### Configuration Phases

MyStation uses a three-phase boot process:

1. **Phase 1: WiFi Setup** - No WiFi configured, create AP
2. **Phase 2: App Setup** - WiFi configured, need station config
3. **Phase 3: Operational** - Fully configured, normal operation

See [Boot Process](boot-process.md) for details.

### Deep Sleep Cycle

Normal operation follows this pattern:

```
Wake Up → Connect WiFi → Fetch Data → Update Display → Deep Sleep
  ↑                                                          │
  └──────────────────────────────────────────────────────────┘
                    (Configured interval)
```

See [Power Management](power-management.md) for implementation.

### Display Modes

Three display modes supported:

- **DISPLAY_MODE_HALF_AND_HALF** (0) - Weather + Departures
- **DISPLAY_MODE_WEATHER_ONLY** (1) - Weather full screen
- **DISPLAY_MODE_DEPARTURE_ONLY** (2) - Departures full screen

See [Display System](display-system.md) for rendering details.

### Configuration Storage

All settings stored in NVS (Non-Volatile Storage):

- WiFi credentials (encrypted namespace)
- Station configuration
- Display preferences
- Update intervals
- Sleep schedule

See [Configuration System](configuration-system.md) for data structures.

## Code Organization

```
mystation/
├── src/                 # Source code
│   ├── main.cpp        # Entry point
│   ├── api/            # API clients
│   ├── config/         # Configuration
│   ├── display/        # Display rendering
│   ├── ota/            # OTA updates
│   ├── sec/            # Security (AES)
│   └── util/           # Utilities
├── include/            # Header files (mirrors src/)
├── lib/                # Libraries
├── test/               # Unit tests
├── data/               # SPIFFS files
├── docs/               # Documentation
└── platformio.ini      # Build configuration
```

See [Project Structure](project-structure.md) for detailed explanation.

## Common Development Tasks

### Adding a New Feature

1. **Plan the feature** - Design and requirements
2. **Create branch** - `git checkout -b feature/my-feature`
3. **Implement** - Write code
4. **Test** - Verify functionality
5. **Document** - Update docs
6. **Submit PR** - Code review

### Debugging Issues

1. **Check serial monitor** - Most issues show here
2. **Verify configuration** - NVS values correct?
3. **Test WiFi** - 2.4 GHz connection stable?
4. **Check hardware** - Wiring, power supply
5. **Review code** - Logic errors, typos
6. **Use logs** - ESP_LOG levels (ERROR, WARN, INFO, DEBUG)

### Modifying Pin Configuration

1. **Edit `include/config/pins.h`**
2. **Update GPIO assignments**
3. **Rebuild and upload**
4. **Verify connections**
5. **Test functionality**

See [Pin Configuration](pin-configuration.md) for details.

### Testing Changes

```bash
# Run native tests
pio test -e native

# Run specific test
pio test -e native -f test_timing_manager

# Upload and monitor
pio run --target upload && pio device monitor
```

See [Testing](testing.md) for test framework details.

## Contributing

### Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
    - Classes: PascalCase
    - Functions: camelCase
    - Constants: UPPER_SNAKE_CASE
    - Variables: camelCase
- **Comments**: Doxygen-style for public APIs

### Git Workflow

1. Fork the repository
2. Create feature branch
3. Make commits (atomic, well-described)
4. Test thoroughly
5. Submit pull request
6. Address review comments

### Documentation

- Update relevant documentation with code changes
- Add examples for new features
- Keep README.md current
- Document breaking changes

## Resources

### Documentation

- 📖 [User Guide](../user-guide/index.md) - End-user documentation
- 📚 [API Reference](../reference/configuration-keys.md) - Configuration keys
- 📋 [Quick Reference](../reference/configuration-phases.md) - Phase guide

### External Resources

- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [Arduino Reference](https://www.arduino.cc/reference/en/)

### Community

- GitHub Issues - Bug reports and feature requests
- Pull Requests - Code contributions
- Discussions - Questions and ideas

## Next Steps

### New to the Project?

1. Read [System Architecture](architecture.md)
2. Understand [Boot Process](boot-process.md)
3. Review [Project Structure](project-structure.md)
4. Set up [Development Environment](development-setup.md)

### Ready to Code?

1. Check open issues for tasks
2. Review [Code Conventions](code-conventions.md)
3. Set up your development environment
4. Start with small improvements
5. Ask questions if stuck

### Want to Contribute?

1. Read contributing guidelines
2. Pick an issue or suggest a feature
3. Discuss approach before major changes
4. Write tests for new functionality
5. Update documentation
6. Submit pull request

---

**Questions?** Check the [Troubleshooting Guide](../user-guide/troubleshooting.md) or open an issue on GitHub.

