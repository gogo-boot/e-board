# Factory Reset

This guide explains how to perform a factory reset on your MyStation device to restore it to default settings.

## What is Factory Reset?

A factory reset will:

- ✅ Erase all saved configuration
- ✅ Clear WiFi network credentials
- ✅ Remove station and transport preferences
- ✅ Reset to initial setup mode
- ✅ Clear all user data from storage

After a factory reset, your device will:

- 🔄 Restart in configuration mode
- 📡 Create WiFi access point (`MyStation-XXXXXXXX`)
- ⚙️ Wait for you to complete initial setup again

## When to Use Factory Reset

### Good Reasons to Reset

- 🏠 **Moving to new location** - Need to configure different WiFi and transport stops
- 🔧 **Troubleshooting** - Device not working properly, want to start fresh
- 🎁 **Giving device to someone else** - Clear your personal settings
- 🔄 **Testing** - Want to go through setup process again
- ⚠️ **Corrupted configuration** - Settings seem broken or inconsistent

### Before You Reset

⚠️ **Note**: Factory reset is permanent! You will need to:

- Reconfigure WiFi connection (have your WiFi password ready)
- Reselect your transport station
- Reconfigure display preferences
- Set up any custom settings again

## Factory Reset Methods

MyStation supports different reset methods depending on your hardware:

### Method 1: Button Reset (ESP32-S3 Only)

If you have an ESP32-S3 board with buttons connected:

#### Step-by-Step

1. **Power off** your MyStation (disconnect USB and battery)

2. **Press and hold Button 1** (GPIO 2)
    - This is typically the leftmost button
    - Keep it pressed during next step

3. **Power on** the device while still holding Button 1
    - Connect USB-C cable OR insert battery
    - Keep holding the button

4. **Watch the serial monitor** (optional)
   ```
   🔵 Reset button detected!
      Hold button for 5 seconds to factory reset...
   ⏱️  Holding... 4 seconds remaining
   ⏱️  Holding... 3 seconds remaining
   ⏱️  Holding... 2 seconds remaining
   ⏱️  Holding... 1 seconds remaining
   ✅ Button held for 5 seconds!
   ```

5. **Continue holding for 5 full seconds**
    - You'll feel the button is still pressed
    - Count to 5 slowly

6. **Release the button** after 5 seconds
   ```
   🔥 ================================
   🔥 FACTORY RESET INITIATED!
   🔥 ================================

   🗑️  Erasing NVS (Non-Volatile Storage)...
   ✅ NVS erased successfully!
   ✅ NVS reinitialized successfully!

   ✨ Factory reset complete!
   ```

7. **Device will restart** in configuration mode
    - WiFi access point will be created
    - Connect to `MyStation-XXXXXXXX` to begin setup

#### Troubleshooting Button Reset

**Button released too early:**

```
🟢 Button released after 2.3 seconds
   (Not long enough for factory reset)
```

- Try again, hold for full 5 seconds

**No reset button message:**

- Button might not be connected to GPIO 2
- Check button wiring
- Try Method 2 or 3 instead

### Method 2: Web Interface Reset (Planned Feature)

> ⚠️ **Coming Soon**: Web-based factory reset from configuration page

When available, you'll be able to:

1. Navigate to `http://mystation.local` or device IP
2. Click "Factory Reset" button
3. Confirm the action
4. Device will reset automatically

### Method 3: Serial Command Reset (Development)

For developers with serial monitor access:

1. **Connect to serial monitor**
   ```bash
   pio device monitor
   ```

2. **Enter reset command** (if implemented)
   ```
   FACTORY_RESET
   ```

3. **Confirm** when prompted

> 💡 This method requires special firmware build with serial commands enabled

### Method 4: Manual NVS Erase (Advanced)

For advanced users with PlatformIO:

1. **Connect device via USB**

2. **Erase flash memory**
   ```bash
   pio run --target erase
   ```

3. **Re-upload firmware**
   ```bash
   pio run --target upload
   pio run --target uploadfs
   ```

4. **Device will start fresh** with no configuration

> ⚠️ **Warning**: This erases ALL data including firmware. You must re-upload firmware after.

## After Factory Reset

### Step 1: Verify Reset Mode

Your device should now:

- ✅ Show "Fresh boot" in serial monitor
- ✅ Create WiFi access point `MyStation-XXXXXXXX`
- ✅ Wait for configuration

### Step 2: Reconfigure Device

Follow the [Quick Start Guide](quick-start.md) to set up your device again:

1. **Connect to WiFi**
    - Join `MyStation-XXXXXXXX` network
    - Configure your home WiFi (2.4 GHz only)

2. **Configure Station**
    - Select transport stop
    - Choose display preferences
    - Set update interval

3. **Verify Operation**
    - Check display updates correctly
    - Confirm WiFi connection works
    - Test any buttons (ESP32-S3)

## Common Questions

### Will I lose my firmware?

**No** - Factory reset only clears configuration data (NVS storage)

- Firmware remains installed
- Web interface files remain
- Only user settings are erased

Exception: Method 4 (Manual NVS Erase) erases everything.

### Will factory reset fix my problem?

**Maybe** - Factory reset helps with:

- ✅ Configuration corruption
- ✅ Wrong WiFi settings
- ✅ Stuck in error state
- ✅ Need to change location/stops

Factory reset won't fix:

- ❌ Hardware problems (wiring, broken display)
- ❌ WiFi network issues (router problems)
- ❌ API service outages
- ❌ Firmware bugs

### Can I back up my settings first?

**Not currently** - There's no built-in backup feature

- Write down your settings before reset:
    - WiFi network name
    - Station name and ID
    - Update interval
    - Display mode preference
    - Sleep schedule (if used)

### How long does factory reset take?

- **Button method**: ~10 seconds (5 second hold + restart)
- **Flash erase method**: ~2-3 minutes (erase + re-upload)

### Can I undo a factory reset?

**No** - Factory reset is permanent

- All settings are erased and cannot be recovered
- You must reconfigure from scratch

### Will my device get a new ID after reset?

**No** - The device ID (used in WiFi AP name) is hardware-based

- Same WiFi AP name: `MyStation-XXXXXXXX`
- Same mDNS name: `mystation.local`

## Troubleshooting Reset Issues

### Factory Reset Doesn't Start

**Symptoms**: Hold button but nothing happens

**Solutions**:

- ✅ Verify you're holding correct button (Button 1, GPIO 2)
- ✅ Check button is properly connected
- ✅ Try power cycling device first
- ✅ Use alternative reset method

### Device Stuck After Reset

**Symptoms**: Device restarts but doesn't enter config mode

**Solutions**:

- ✅ Check serial monitor for error messages
- ✅ Power cycle the device completely
- ✅ Try factory reset again
- ✅ Use Manual NVS Erase method (Method 4)

### Can't Connect to Config WiFi After Reset

**Symptoms**: No `MyStation-XXXXXXXX` network visible

**Solutions**:

- ✅ Wait 30 seconds after power on
- ✅ Check serial monitor - is AP started?
- ✅ Look for network with different suffix
- ✅ Try resetting router/phone WiFi
- ✅ Check device is powered (LED on)

## Alternative: Reconfiguration Without Reset

If you just want to change settings without erasing everything:

1. **Access web interface**
    - `http://mystation.local` or device IP

2. **Update settings**
    - Change WiFi network
    - Select different station
    - Modify update interval

3. **Save changes**
    - Click "Save Settings"
    - Device will use new configuration

> 💡 This preserves working settings while allowing changes

## Need More Help?

- 🚀 [Quick Start Guide](quick-start.md) - Complete setup instructions after reset
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions

---

**Still having issues?** Check the [Troubleshooting Guide](troubleshooting.md) or review
the [Developer Documentation](../developer-guide/index.md) for technical details.

