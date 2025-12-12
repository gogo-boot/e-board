#pragma once

// =============================================================================
// Production/Debug Configuration
// =============================================================================
// Production flag (set via build flags in platformio.ini)
// -D PRODUCTION=0 for debug builds
// -D PRODUCTION=1 for production builds
#ifndef PRODUCTION
#define PRODUCTION 0  // Default to debug mode if not specified
#endif

// Helper macros for cleaner code
#define IS_DEBUG (PRODUCTION == 0)
#define IS_PRODUCTION (PRODUCTION == 1)

// Debug-only code blocks
#if IS_DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif

// Production-only code blocks
#if IS_PRODUCTION
#define PRODUCTION_ONLY(x) x
#else
#define PRODUCTION_ONLY(x)
#endif

// =============================================================================
// Board Detection and Configuration
// =============================================================================
// Board type enum for runtime checks
enum class BoardType {
    UNKNOWN = 0,
    ESP32_C3 = 1,
    ESP32_S3 = 2
};

// Detect which board we're building for (set via platformio.ini build_flags)
#if defined(BOARD_ESP32_C3)
#define CURRENT_BOARD BoardType::ESP32_C3
#define BOARD_NAME "ESP32-C3"
#define SHOW_BATTERY_STATUS 0

#elif defined(BOARD_ESP32_S3)
#define CURRENT_BOARD BoardType::ESP32_S3
#define BOARD_NAME "ESP32-S3"
#define SHOW_BATTERY_STATUS 1

#endif

// =============================================================================
// Debug Display Features
// =============================================================================
#define SHOW_NEXT_WAKEUP_TIME (PRODUCTION == 0)
#define SHOW_BUILD_INFO (PRODUCTION == 0)
#define SHOW_GIT_TAG (PRODUCTION == 0)

// =============================================================================
// Build Information
// =============================================================================
// Build timestamp (set via build flags)
#ifndef BUILD_TIME
#define BUILD_TIME 0
#endif

// Version (set via build flags from git tags)
#ifndef VERSION
#define VERSION "unknown"
#endif
