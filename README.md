# MyStation E-Board

> ESP32-C3 powered public transport departure board with e-paper display

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-blue.svg)](https://platformio.org/)
[![ESP32-C3](https://img.shields.io/badge/ESP32--C3-Supported-green.svg)](https://www.espressif.com/en/products/socs/esp32-c3)

## ✨ Features

- 🚌 **Real-time departures** from German public transport (RMV API)
- 🌤️ **Weather information** from German Weather Service (DWD)
- 📱 **Mobile-friendly web configuration** with privacy focus
- 🔋 **Ultra-low power** with intelligent deep sleep scheduling
- 📡 **WiFi auto-discovery** and mDNS support
- 🔒 **Privacy-conscious** - all processing happens locally
- 🎨 **E-paper display** optimized for outdoor visibility

## 🚀 Quick Start

### 1. Hardware Setup
Connect ESP32-C3 Super Mini to e-paper display:
```
ESP32-C3    →    E-Paper Display
GPIO 2      →    BUSY
GPIO 3      →    CS
GPIO 4      →    SCK
GPIO 6      →    SDI (MOSI)
GPIO 8      →    RES
GPIO 9      →    DC
3.3V        →    VCC
GND         →    GND
```

### 2. Software Setup
```bash
# Clone repository
git clone <repository-url>
cd e-board

# Install PlatformIO and build
pio run --target upload
pio run --target uploadfs
```

### 3. Configuration
1. Connect to `MyStation-XXXXXXXX` WiFi network
2. Open browser to configure your location and transport stops
3. Device automatically switches to operational mode

**📖 [Complete Setup Guide](./docs/quick-start.md)**

## 📚 Documentation

| Topic | Description |
|-------|-------------|
| **[📖 Quick Start](./docs/quick-start.md)** | Get running in 5 minutes |
| **[🔧 Hardware Setup](./docs/hardware-setup.md)** | Wiring and pin definitions |
| **[💻 Software Setup](./docs/software-setup.md)** | Development environment |
| **[🔑 API Keys](./docs/api-keys.md)** | Required API configuration |
| **[⚙️ Configuration](./docs/configuration.md)** | Detailed options |
| **[🛠️ Troubleshooting](./docs/troubleshooting.md)** | Common issues |

**[📚 Complete Documentation](./docs/)**

## 🏗️ Architecture

```mermaid
flowchart TD
    User((User)) -->|WiFi| ESP32[ESP32-C3 Device]
    ESP32 -->|Location| Google[Google Geolocation API]
    ESP32 -->|Departures| RMV[RMV Transport API]  
    ESP32 -->|Weather| DWD[DWD Weather API]
    ESP32 -->|Display| EPD[E-Paper Display]
```

### Key Components
- **ESP32-C3 Super Mini**: Main controller with WiFi
- **E-Paper Display**: Low-power outdoor-readable screen  
- **APIs**: Google (location), RMV (transport), DWD (weather)
- **Deep Sleep**: Battery-optimized operation
- **Web Interface**: Configuration and status

## 🔋 Power Management

- **Active Mode**: ~100mA (during data fetch)
- **Deep Sleep**: <50μA (between updates)  
- **Battery Life**: 2-4 weeks on 2000mAh (5-min intervals)
- **Smart Scheduling**: Reduced updates during night hours

## 🌍 Coverage

- **Transport**: Hesse, Germany (RMV network)
  - Frankfurt, Wiesbaden, Kassel, Darmstadt, etc.
  - Trains, buses, trams, S-Bahn
- **Weather**: Germany and surrounding areas (DWD)
- **Extensible**: Adapt for other regions/APIs

## 🛠️ Development

### Prerequisites
- PlatformIO IDE (VS Code recommended)
- ESP32-C3 development board
- API keys (Google, RMV)

### Project Structure
```
├── docs/              # Documentation
├── src/               # Source code
│   ├── api/          # External API integrations
│   ├── config/       # Configuration management  
│   ├── util/         # Utilities and helpers
│   └── main.cpp      # Main application
├── data/             # Web interface files
└── platformio.ini    # Build configuration
```

### Contributing
1. Fork the repository
2. Create feature branch
3. Test thoroughly
4. Submit pull request

**[🔧 Development Guide](./docs/development.md)**

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

**🚀 Ready to build your own departure board?**  
**[Start with the Quick Start Guide →](./docs/quick-start.md)**
