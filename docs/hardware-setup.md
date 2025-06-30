# Hardware Setup

## ESP32-C3 Super Mini

### Pin Layout
The ESP32-C3 Super Mini is a compact development board with the following pin assignments:

#### General Purpose Pins
| Pin | GPIO | Function | Notes |
|-----|------|----------|-------|
| A0  | 0    | Analog/Digital | |
| A1  | 1    | Analog/Digital | |
| A2  | 2    | Digital | Used for E-Paper BUSY |
| A3  | 3    | Digital | Used for E-Paper CS |
| A4  | 4    | Digital | User Button (shares with SCK) |
| A5  | 5    | Digital | Shares with MISO |
| GPIO10 | 10 | Digital | General purpose |

#### Communication Pins
| Pin | GPIO | Function | Notes |
|-----|------|----------|-------|
| SCK | 4    | SPI Clock | Shared with A4/User Button |
| MISO| 5    | SPI MISO | Shared with A5 |
| MOSI| 6    | SPI MOSI | Used for E-Paper SDI |
| SS  | 7    | SPI Chip Select | General purpose |
| SDA | 8    | I2C Data | Used for E-Paper RES |
| SCL | 9    | I2C Clock | Used for E-Paper DC |
| RX  | 20   | UART Receive | Serial communication |
| TX  | 21   | UART Transmit | Serial communication |

## E-Paper Display Wiring

### Required Connections
The e-paper display requires **6 GPIO pins + 3.3V + Ground** (total 8 pins):

| E-Paper Pin | ESP32-C3 Pin | GPIO | Function |
|-------------|--------------|------|----------|
| BUSY        | A2           | 2    | Busy signal |
| CS          | A3           | 3    | Chip Select |
| SCK         | SCK          | 4    | SPI Clock |
| SDI         | MOSI         | 6    | SPI Data In |
| RES         | SDA          | 8    | Reset |
| DC          | SCL          | 9    | Data/Command |
| 3.3V        | 3.3V         | -    | Power supply |
| GND         | GND          | -    | Ground |

### Wiring Diagram
```
ESP32-C3 Super Mini          E-Paper Display
┌─────────────────┐         ┌─────────────────┐
│ 3.3V      ●─────┼─────────┼─● 3.3V          │
│ GND       ●─────┼─────────┼─● GND           │
│ GPIO 2    ●─────┼─────────┼─● BUSY          │
│ GPIO 3    ●─────┼─────────┼─● CS            │
│ GPIO 4    ●─────┼─────────┼─● SCK           │
│ GPIO 6    ●─────┼─────────┼─● SDI           │
│ GPIO 8    ●─────┼─────────┼─● RES           │
│ GPIO 9    ●─────┼─────────┼─● DC            │
└─────────────────┘         └─────────────────┘
```

### Pin Conflicts
⚠️ **Important Pin Sharing Notes:**
- **GPIO 4**: Shared between SPI SCK and User Button
- **GPIO 5**: Shared between SPI MISO and A5
- **GPIO 8**: Shared between I2C SDA and E-Paper RES
- **GPIO 9**: Shared between I2C SCL and E-Paper DC

## Power Supply

### Power Options
1. **USB-C**: 5V input, regulated to 3.3V on-board
2. **Battery**: 3.7V LiPo battery (JST connector if available)
3. **External 3.3V**: Direct 3.3V supply to 3.3V pin

### Power Consumption
- **Active mode**: ~80-120mA (WiFi + display update)
- **Deep sleep**: ~10-50μA (depending on configuration)
- **Estimated battery life**: 
  - 2000mAh battery: 2-4 weeks (5-minute update intervals)
  - 1000mAh battery: 1-2 weeks (5-minute update intervals)

## Assembly Tips

### Soldering
1. Use flux for clean solder joints
2. Start with power connections (3.3V, GND)
3. Double-check polarity before powering on
4. Test continuity with multimeter

### Enclosure Considerations
- Ensure antenna clearance for WiFi
- Provide access to USB-C for programming
- Consider heat dissipation during operation
- Protect against moisture if used outdoors

### Testing
1. **Power test**: Verify 3.3V supply
2. **SPI test**: Check e-paper display communication
3. **WiFi test**: Verify network connectivity
4. **Full system test**: Run complete firmware

## Troubleshooting Hardware Issues

### Display Not Working
- Check all 8 connections (6 GPIO + power)
- Verify 3.3V power supply stability
- Ensure proper SPI timing (check SCK signal)

### WiFi Connection Issues
- Check antenna clearance
- Verify power supply stability during transmission
- Ensure proper grounding

### Power Issues
- Measure actual power consumption vs. expected
- Check for shorts or incorrect wiring
- Verify deep sleep functionality

## Next Steps
After hardware setup, proceed to:
- [Software Setup](./software-setup.md) - Development environment
- [Quick Start Guide](./quick-start.md) - Initial configuration
