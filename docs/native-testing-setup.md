# Native Testing Setup for MyStation

## Overview

This document describes the native testing setup that allows you to run unit tests for ESP32 code on your local
development machine without requiring actual hardware.

## Setup Components

### 1. PlatformIO Configuration (`platformio.ini`)

The `[env:native]` environment is configured to:

- Use the `native` platform (runs on your local machine)
- Override the Arduino framework to avoid ESP32 dependencies
- Build only the specific source files needed for testing
- Include mock headers from `test/mocks/`

```ini
[env:native]
platform = native
framework =                    # Override to prevent Arduino framework inheritance
lib_deps =                     # Override to prevent library dependencies
test_build_src = yes           # Enable building source files for tests
build_src_filter =
    -<*>                       # Exclude all files by default
    +<util/timing_manager.cpp> # Include only the file being tested
test_filter = test_timing_manager
build_flags =
    -std=c++11
    -Iinclude                  # Include production headers
    -Itest/mocks               # Include mock headers (takes precedence)
    -DNATIVE_TEST              # Define flag for conditional compilation
lib_compat_mode = off
```

### 2. Mock Files Structure

All mock files are located in `test/mocks/`:

```
test/mocks/
├── Arduino.h              # Mock Arduino core
├── Preferences.h          # Mock ESP32 Preferences library
├── esp32_mocks.h          # Common ESP32 mocks (logging, String class)
├── esp_log.h              # Mock ESP logging
├── esp_sleep.h            # Mock ESP sleep functions
├── time_manager.h         # Mock TimeManager class
└── config_manager.cpp     # Mock ConfigManager implementation
```

### 3. Mock Implementations

#### Arduino String Class (`esp32_mocks.h`)

A mock String class that inherits from `std::string` and provides Arduino-compatible methods:

- `indexOf(char c)`
- `substring(int start, int end)`
- `toInt()`, `toFloat()`
- `length()`, `c_str()`

#### ESP32 Logging (`esp32_mocks.h`)

Mock logging macros that output to stdout:

- `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`, `ESP_LOGD`, `ESP_LOGV`
- `RTC_DATA_ATTR` (no-op for native)

#### Preferences Library (`Preferences.h`)

A complete mock of ESP32's Preferences library using `std::map` for storage:

- `begin()`, `end()`, `clear()`
- Getters: `getInt()`, `getFloat()`, `getString()`, etc.
- Setters: `putInt()`, `putFloat()`, `putString()`, etc.

#### ConfigManager (`config_manager.cpp`)

A simplified mock implementation that:

- Defines `rtcConfig` with default values
- Implements all public methods
- Uses `#ifdef NATIVE_TEST` to compile only for native tests

### 4. Conditional Compilation in Production Code

The production code uses conditional compilation to switch between real and mock headers:

```cpp
#ifdef NATIVE_TEST
#include "time_manager.h"      // Mock from test/mocks/
#include "esp_log.h"           // Mock from test/mocks/
#else
#include "util/time_manager.h" // Real header from include/
#include <esp_log.h>           // Real ESP-IDF header
#endif
```

### 5. Test Structure

Tests are organized in `test/test_timing_manager/`:

```
test/test_timing_manager/
├── test_sleep_duration.cpp    # Test cases
└── mocks/
    └── config_manager.cpp     # Mock copied here for test build
```

## Running Tests

### Run all native tests:

```bash
pio test -e native
```

### Run with verbose output:

```bash
pio test -e native -v
```

### Run with extra verbose output:

```bash
pio test -e native -vvv
```

## Test Output Example

```
[INFO][TIMING_MGR] Calculating sleep duration - Display mode: 1, Current time: 1761773673
[INFO][TIMING_MGR] Last updates - Weather: 0 seconds, Departure: 0 seconds
[INFO][TIMING_MGR] Weather interval: 1 hours (3600 seconds), Next weather update: 1761773673
[INFO][TIMING_MGR] Only weather update needed at: 1761773673 seconds
[INFO][TIMING_MGR] Final sleep duration: 30 seconds (0 minutes)
test/test_timing_manager/test_sleep_duration.cpp:111:test_getNextSleepDurationSeconds_weather_only_mode:PASS

6 Tests 0 Failures 0 Ignored
OK
```

## Key Benefits

1. **Fast Testing**: No need to flash firmware to hardware
2. **Easy Debugging**: Use standard C++ debugging tools
3. **CI/CD Ready**: Can run in automated build pipelines
4. **Isolation**: Test business logic without hardware dependencies
5. **No Code Changes**: Production code remains clean with minimal conditional compilation

## Adding New Tests

To add tests for a new module:

1. Create a new test directory: `test/test_<module_name>/`
2. Create test file: `test_<feature>.cpp`
3. Add required mocks to `test/mocks/` if needed
4. Update `platformio.ini` build_src_filter to include the source file
5. Run tests with `pio test -e native -v`

## Troubleshooting

### "Undefined symbols for architecture"

- Check that source files are included in `build_src_filter`
- Verify mock implementations exist for all dependencies

### "file not found" errors

- Ensure mock headers are in `test/mocks/`
- Check that `-Itest/mocks` is in build_flags
- Verify conditional includes in source files

### Framework inheritance issues

- Make sure `framework =` is explicitly set in `[env:native]`
- Set `lib_deps =` to override inherited dependencies

## Future Improvements

- Add more comprehensive test cases
- Mock additional ESP32 libraries as needed
- Add integration with CI/CD pipeline
- Add code coverage reporting

