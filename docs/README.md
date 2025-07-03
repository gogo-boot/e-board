# MyStation E-Board Documentation

Comprehensive documentation for the ESP32-C3 powered public transport departure board.

## 📚 Documentation Index

### Getting Started
- [Hardware Setup](./hardware-setup.md) - ESP32-C3 Super Mini wiring and e-paper display connections
- [Software Setup](./software-setup.md) - Development environment and flashing instructions
- [Quick Start Guide](./quick-start.md) - Get up and running in 5 minutes

### Configuration
- [Configuration Guide](./configuration.md) - Web interface setup and options
- [Configuration Keys Mapping](./configuration-keys-mapping.md) - Complete key mapping across all layers
- [Configuration Keys Quick Reference](./configuration-keys-quick-reference.md) - Developer quick reference
- [Configuration Data Flow](./configuration-data-flow.md) - Complete save/load process documentation
- [Configuration Flowchart](./configuration-flowchart.md) - Visual process flow diagrams
- [API Keys Setup](./api-keys.md) - Required API keys and configuration
- [Deep Sleep Configuration](./deep-sleep.md) - Power management and sleep scheduling

### Technical Reference
- [Pin Definitions](./pin-definitions.md) - ESP32-C3 Super Mini pin assignments
- [API Reference](./api-reference.md) - Web endpoints and data structures
- [Code Architecture](./architecture.md) - Project structure and modules

### Advanced Topics
- [OTA Updates](./ota-updates.md) - Over-the-air firmware updates
- [Troubleshooting](./troubleshooting.md) - Common issues and solutions
- [Development](./development.md) - Contributing and extending the project

### External Resources
- [DWD Weather API](./external-apis.md#dwd-weather-api) - German Weather Service integration
- [RMV Transport API](./external-apis.md#rmv-transport-api) - Regional transport data
- [Google Geolocation API](./external-apis.md#google-api) - Location services

---

## Project Overview

The MyStation E-Board is a battery-powered, WiFi-enabled departure board that displays:
- 🚌 Real-time public transport departures
- 🌤️ Current weather information
- ⚡ Intelligent power management with deep sleep
- 📱 Mobile-friendly web configuration interface

### Key Features
- **Privacy-conscious**: All data processing happens locally
- **Low power**: Deep sleep between updates for battery operation
- **Modular**: Clean code architecture with separated concerns
- **User-friendly**: Web-based configuration with auto-discovery
- **Reliable**: Persistent configuration and robust error handling
