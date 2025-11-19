# Hardware Assembly

This guide will walk you through physically assembling your MyStation e-paper display.

## Required Components

### Core Components

- **ESP32-C3 Super Mini** OR **ESP32-S3** development board
- **7.5" E-Paper Display** (GDEY075T7, 800x480 resolution)
- **USB-C Cable** for programming and charging
- **3.7V LiPo Battery** (1000-2500mAh recommended)

### Optional Components

- **Push Buttons** (3x, ESP32-S3 only) for display mode control
- **Enclosure** for protecting the electronics
- **Battery JST Connector** if not built-in

## Tools Needed

- Soldering iron and solder
- Wire strippers
- Multimeter (for testing connections)
- Small Phillips screwdriver (for enclosure, if used)

## Pin Connections

### ESP32-C3 Super Mini Wiring

The ESP32-C3 requires **8 connections** to the e-paper display:

| E-Paper Pin | Function     | ESP32-C3 Pin | GPIO | Wire Color Suggestion |
|-------------|--------------|--------------|------|-----------------------|
| BUSY        | Busy Signal  | A2           | 2    | Yellow                |
| CS          | Chip Select  | A3           | 3    | Orange                |
| SCK         | SPI Clock    | SCK          | 4    | Green                 |
| SDI (MOSI)  | SPI Data     | MOSI         | 6    | Blue                  |
| RES         | Reset        | SDA          | 8    | Purple                |
| DC          | Data/Command | SCL          | 9    | Gray                  |
| VCC (3.3V)  | Power        | 3.3V         | -    | Red                   |
| GND         | Ground       | GND          | -    | Black                 |

### ESP32-S3 Wiring

The ESP32-S3 requires **8 connections** to the e-paper display:

| E-Paper Pin | Function     | ESP32-S3 Pin | GPIO | Wire Color Suggestion |
|-------------|--------------|--------------|------|-----------------------|
| BUSY        | Busy Signal  | GPIO 4       | 4    | Yellow                |
| CS          | Chip Select  | GPIO 44      | 44   | Orange                |
| SCK         | SPI Clock    | GPIO 7       | 7    | Green                 |
| SDI (MOSI)  | SPI Data     | GPIO 9       | 9    | Blue                  |
| RES         | Reset        | GPIO 38      | 38   | Purple                |
| DC          | Data/Command | GPIO 10      | 10   | Gray                  |
| VCC (3.3V)  | Power        | 3.3V         | -    | Red                   |
| GND         | Ground       | GND          | -    | Black                 |

## Wiring Diagram - ESP32-C3

```
ESP32-C3 Super Mini          E-Paper Display (GDEY075T7)
┌─────────────────┐         ┌─────────────────────────────┐
│                 │         │                             │
│ 3.3V      ●─────┼─────────┼─● VCC (3.3V)                │
│ GND       ●─────┼─────────┼─● GND                       │
│                 │         │                             │
│ GPIO 2    ●─────┼─────────┼─● BUSY                      │
│ GPIO 3    ●─────┼─────────┼─● CS                        │
│ GPIO 4    ●─────┼─────────┼─● SCK                       │
│ GPIO 6    ●─────┼─────────┼─● SDI (MOSI)                │
│ GPIO 8    ●─────┼─────────┼─● RES                       │
│ GPIO 9    ●─────┼─────────┼─● DC                        │
│                 │         │                             │
└─────────────────┘         └─────────────────────────────┘
```

## Assembly Steps

### Step 1: Prepare the Display

1. **Unpack the e-paper display carefully**
    - Handle by the edges to avoid touching the display surface
    - Remove any protective film

2. **Identify the connector pins**
    - Most displays have labeled pins on the ribbon cable or PCB
    - Verify pin labels match the wiring table above

### Step 2: Prepare Wires

1. **Cut wires to appropriate length**
    - Measure distance between ESP32 and display
    - Add 2-3 cm extra for strain relief
    - Recommended: 10-15 cm for most setups

2. **Strip wire ends**
    - Strip 3-4 mm from each end
    - Twist strands tightly

3. **Tin wire ends (optional)**
    - Apply a small amount of solder to wire ends
    - Makes soldering easier and more reliable

### Step 3: Solder Connections

#### For ESP32-C3 Super Mini

1. **Start with Ground and Power**
    - Solder GND (black wire)
    - Solder 3.3V (red wire)
    - Test continuity with multimeter

2. **Solder Signal Pins**
    - Follow the pin mapping table above
    - Double-check each connection before soldering
    - Keep wires neat and organized

3. **Inspect Solder Joints**
    - Each joint should be shiny and cone-shaped
    - No cold solder joints (dull, grainy appearance)
    - No solder bridges between adjacent pins

#### For ESP32-S3

Follow the same process using the ESP32-S3 pin mapping.

### Step 4: Add Button Controls (ESP32-S3 Only)

If you want physical button controls:

**Button Wiring:**

| Button Function     | ESP32-S3 GPIO | Connection   |
|---------------------|---------------|--------------|
| Half & Half Mode    | GPIO 2        | Button → GND |
| Weather Only Mode   | GPIO 3        | Button → GND |
| Departure Only Mode | GPIO 5        | Button → GND |

**Wiring Each Button:**

1. One side of button → GPIO pin
2. Other side of button → GND
3. Internal pull-up resistor enabled in software (no external resistor needed)

> 💡 **Tip**: Use momentary push buttons (normally open)

### Step 5: Battery Connection

1. **Identify battery connector**
    - Most ESP32 boards have a JST connector for LiPo batteries
    - Check polarity: Red (+), Black (-)

2. **Connect battery**
    - ⚠️ **IMPORTANT**: Verify polarity before connecting!
    - Wrong polarity can damage the board
    - Match red to red, black to black

3. **Test with multimeter first**
    - Check battery voltage: should be 3.7-4.2V
    - Verify polarity matches board connector

### Step 6: Test Connections

**Before powering on:**

1. **Visual Inspection**
    - Check all solder joints
    - Verify no short circuits between adjacent pins
    - Ensure wires are properly routed

2. **Continuity Test**
    - Use multimeter in continuity mode
    - Test each connection from ESP32 pin to display pin
    - Verify GND connections

3. **Power Test**
    - Connect USB-C cable (no battery yet)
    - ESP32 LED should light up
    - No smoke or burning smell

4. **Initial Boot**
    - Upload firmware (see [Quick Start](quick-start.md))
    - Display should initialize and show test pattern

### Step 7: Final Assembly (Optional)

If using an enclosure:

1. **Plan component layout**
    - Display in front panel opening
    - ESP32 behind display
    - Battery in compartment

2. **Secure components**
    - Use standoffs or mounting brackets
    - Ensure display is firmly mounted
    - Leave access to USB-C port

3. **Cable management**
    - Route wires neatly
    - Use cable ties or clips
    - Avoid stress on solder joints

4. **Test before closing**
    - Power on and verify display works
    - Check WiFi connectivity
    - Verify buttons work (if installed)

## Wiring Best Practices

### Do's ✅

- ✅ Use consistent wire colors for easier troubleshooting
- ✅ Test each connection as you go
- ✅ Leave some slack in wires for strain relief
- ✅ Use heat shrink tubing to insulate connections
- ✅ Double-check pin numbers before soldering
- ✅ Keep wires organized and labeled

### Don'ts ❌

- ❌ Don't apply excessive heat (damages components)
- ❌ Don't create solder bridges between pins
- ❌ Don't pull on wires (stress on solder joints)
- ❌ Don't mix up 3.3V and 5V (can damage e-paper display)
- ❌ Don't connect battery backwards
- ❌ Don't touch e-paper display surface with fingers

## Troubleshooting Assembly Issues

### Display Not Responding

- **Check power connections**: Verify 3.3V and GND are connected
- **Check SPI connections**: SCK, MOSI must be correct
- **Check control signals**: CS, DC, RES, BUSY must be connected
- **Verify pin numbers**: Match GPIO numbers, not physical pin positions

### Display Shows Garbage/Noise

- **Check all control pins**: DC, RES, CS, BUSY
- **Verify SPI pins**: SCK and MOSI especially critical
- **Check for loose connections**: Re-solder suspect joints
- **Test continuity**: Use multimeter to verify all connections

### ESP32 Won't Power On

- **Check USB cable**: Try different cable
- **Check battery polarity**: Reverse polarity can cause damage
- **Look for short circuits**: Inspect solder joints
- **Test without display**: Disconnect display, test ESP32 alone

### Buttons Don't Work (ESP32-S3)

- **Check button wiring**: One side to GPIO, other to GND
- **Test button continuity**: Button should close circuit when pressed
- **Verify GPIO numbers**: Must match code expectations
- **Check for shorts**: Buttons should only connect when pressed

## Safety Notes

⚠️ **Important Safety Information:**

- **LiPo Battery Safety**:
    - Never short circuit battery terminals
    - Don't puncture or damage battery
    - Charge at appropriate rate (1C max)
    - Monitor temperature during charging
    - Store at 50-60% charge if not using

- **Soldering Safety**:
    - Use in well-ventilated area
    - Don't touch hot iron tip
    - Use proper ventilation/fume extraction
    - Wash hands after handling solder

- **Electrical Safety**:
    - Disconnect power before making connections
    - Verify polarity before connecting battery
    - Don't exceed voltage ratings (3.3V for display)

## What's Next?

Once your hardware is assembled and tested:

1. 📖 [Install Firmware](quick-start.md#step-2-install-firmware-5-minutes)
2. ⚙️ [Configure WiFi and Station](quick-start.md#step-3-wifi-configuration-3-minutes)
3. 📱 [Start Using MyStation](quick-start.md#step-5-verify-operation-2-minutes)

Need help? Check the [Troubleshooting Guide](troubleshooting.md) for common issues and solutions.

