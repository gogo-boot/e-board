# Build for ESP32-S3

platformio run -e esp32-s3-base

# Upload firmware

platformio run -e esp32-s3-base -t upload

# Monitor serial output

platformio device monitor -b 115200

```

## Monitoring

Watch for these key log messages:

- `Configuration Phase: X` - Current phase
- `WiFi and internet validation successful` - Phase 1 complete
- `WiFi validation failed - reverting to Phase 1` - Auto-recovery
- `Configuration loaded from NVS to RTC memory` - Boot initialization
- `Internet access confirmed` - Connectivity OK

