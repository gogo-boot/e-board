# MyStation Documentation

Welcome to the MyStation E-Board documentation! This guide will help you get started, use, and understand your public
transport departure board.

## 📖 Documentation Sections

### For Users

If you want to build, set up, and use MyStation:

**[👋 User Guide](user-guide/index.md)** - Complete guide for end users

- [Quick Start](user-guide/quick-start.md) - Get running in 15 minutes
- [Hardware Assembly](user-guide/hardware-assembly.md) - Physical setup and wiring
- [Understanding the Display](user-guide/understanding-display.md) - What information is shown
- [Button Controls](user-guide/button-controls.md) - Using physical buttons (ESP32-S3)
- [Factory Reset](user-guide/factory-reset.md) - Reset to defaults
- [Troubleshooting](user-guide/troubleshooting.md) - Common issues and solutions

### For Developers

If you want to understand the code, contribute, or modify MyStation:

**[🔧 Developer Guide](developer-guide/index.md)** - Technical documentation

- [Architecture Overview](developer-guide/index.md) - System design
- [Boot Process](developer-guide/boot-process.md) - Detailed boot flow
- [Configuration System](configuration.md) - How settings are stored
- [Display System](display-layout-overview.md) - E-paper rendering
- [API Integration](api-keys.md) - External API usage
- [Hardware Setup](hardware-setup.md) - Pin configuration and specs
- [Development Setup](software-setup.md) - PlatformIO environment
- [Testing](native-testing-setup.md) - Unit tests and mocks

### Quick Reference

**[📋 Reference Guides](reference/)** - Quick lookup

- [Configuration Keys](reference/configuration-keys-quick-reference.md) - All settings
- [Configuration Phases](reference/configuration-phase-quick-reference.md) - Boot phases

## 🚀 Quick Links

### New to MyStation?

1. **[What is MyStation?](user-guide/index.md)** - Overview and features
2. **[Quick Start Guide](user-guide/quick-start.md)** - Build and configure in 15 minutes
3. **[Troubleshooting](user-guide/troubleshooting.md)** - Common issues

### Common Tasks

- **[First Time Setup](user-guide/quick-start.md#step-3-wifi-configuration-3-minutes)** - WiFi and station configuration
- **[Change WiFi Network](user-guide/quick-start.md#need-to-reconfigure)** - Reconfigure WiFi
- **[Factory Reset](user-guide/factory-reset.md)** - Reset all settings
- **[Understanding Display](user-guide/understanding-display.md)** - Read the screen

### Developer Tasks

- **[Development Environment](software-setup.md)** - Set up PlatformIO
- **[Boot Process](developer-guide/boot-process.md)** - How the system starts
- **[Pin Configuration](hardware-setup.md)** - GPIO assignments
- **[API Integration](api-keys.md)** - Add API keys

## 📦 What's Included

MyStation is a complete e-paper display system:

- ✅ **Real-time Transport Data** - RMV (German public transport)
- ✅ **Weather Information** - DWD (German Weather Service)
- ✅ **WiFi Configuration** - Web-based setup
- ✅ **OTA Updates** - Wireless firmware updates
- ✅ **Power Management** - Intelligent deep sleep
- ✅ **Multiple Display Modes** - Weather, departures, or both

## 🔧 Hardware Requirements

### Required

- ESP32-C3 Super Mini OR ESP32-S3 development board
- 7.5" e-paper display (GDEY075T7, 800x480)
- USB-C cable
- WiFi network (2.4 GHz - **5 GHz not supported**)

### Optional

- 3.7V LiPo battery (1000-2500mAh)
- Physical buttons (ESP32-S3 only)
- Enclosure

**See:** [Hardware Assembly](user-guide/hardware-assembly.md) for wiring details

## ⚠️ Important Notes

### WiFi Requirements

> **MyStation only works with 2.4 GHz WiFi networks.** 5 GHz networks are not supported due to higher energy
> consumption.

### Button Support

> **Physical buttons are only available on ESP32-S3** boards. The ESP32-C3 Super Mini does not have button support
> configured.

### Regional Support

> **Public transport data is for German transit systems** (RMV API). Weather data uses German Weather Service (DWD).

## 📚 Documentation Structure

```
docs/
├── user-guide/          # End-user documentation
│   ├── index.md        # User guide overview
│   ├── quick-start.md  # Getting started
│   ├── hardware-assembly.md
│   ├── understanding-display.md
│   ├── button-controls.md
│   ├── factory-reset.md
│   └── troubleshooting.md
│
├── developer-guide/     # Developer documentation
│   ├── index.md        # Developer overview
│   └── boot-process.md # Boot flow details
│
├── reference/           # Quick reference
│   ├── configuration-keys-quick-reference.md
│   └── configuration-phase-quick-reference.md
│
├── archive/             # Historical documents
│   └── [old implementation notes]
│
└── [core documentation files]
    ├── configuration.md
    ├── display-layout-overview.md
    ├── api-keys.md
    └── hardware-setup.md
```

## 🎯 Getting Started

### I want to use MyStation

1. Read [User Guide Overview](user-guide/index.md)
2. Follow [Quick Start Guide](user-guide/quick-start.md)
3. Refer to [Troubleshooting](user-guide/troubleshooting.md) if needed

### I want to develop MyStation

1. Read [Developer Guide](developer-guide/index.md)
2. Understand [Boot Process](developer-guide/boot-process.md)
3. Set up [Development Environment](software-setup.md)
4. Review [Architecture](developer-guide/index.md)

## 🆘 Getting Help

### Troubleshooting

Most issues are covered in the [Troubleshooting Guide](user-guide/troubleshooting.md):

- WiFi connection problems
- Display not updating
- Battery issues
- Button problems (ESP32-S3)

### Common Solutions

1. **Check WiFi**: Must be 2.4 GHz network
2. **Check wiring**: Verify pin connections
3. **Check serial monitor**: Error messages show here
4. **Try factory reset**: [Factory Reset Guide](user-guide/factory-reset.md)

### Still Stuck?

- Check GitHub issues for similar problems
- Review serial monitor output
- Verify all prerequisites met
- Check hardware connections

## 🔗 External Resources

- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [RMV API](https://www.rmv.de/auskunft/bin/jp/query.exe/dn)

## 📄 License

[Add your license information here]

## 🤝 Contributing

Contributions are welcome! Please:

1. Read the [Developer Guide](developer-guide/index.md)
2. Check existing issues
3. Follow code conventions
4. Write tests for new features
5. Update documentation

---

**Ready to get started?** Head to the [Quick Start Guide](user-guide/quick-start.md)!

