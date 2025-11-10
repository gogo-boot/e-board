#pragma once

// --- ESP32-C3 Super Mini Pin Definitions ---
namespace Pins {
#ifdef BOARD_ESP32_C3
    constexpr int A0 = 0; //
    constexpr int A1 = 1; //
    constexpr int A2 = 2; // GPIO 2
    constexpr int A3 = 3; // GPIO 3
    constexpr int A4 = 4; // User Button (shares with SCK)
    constexpr int SCK = 4; // SPI SCK
    constexpr int A5 = 5; // shares with MISO
    constexpr int MISO = 5; // SPI MISO
    constexpr int MOSI = 6; // SPI MOSI
    constexpr int SS = 7; // SPI SS (Chip Select)
    constexpr int SDA = 8; // I2C SDA
    constexpr int SCL = 9; // I2C SCL
    constexpr int GPIO10 = 10; // GPIO 10 (can be used for other purposes)
    constexpr int RX = 20; // UART RX
    constexpr int TX = 21; // UART TX

    //E-Paper Display Pins requires 6 Pins, 3.3 V, Ground. total 8 Pins used
    constexpr int EPD_BUSY = 2; // E-Paper Busy
    constexpr int EPD_CS = 3; // E-Paper CS
    constexpr int EPD_SCK = 4; // E-Paper SCK
    constexpr int EPD_SDI = 6; // E-Paper SDI
    constexpr int EPD_RES = 8; // E-Paper RES
    constexpr int EPD_DC = 9; // E-Paper D/C

#elif defined(BOARD_ESP32_S3)
    constexpr int EPD_BUSY = 4; // E-Paper Busy
    constexpr int EPD_CS = 44; // E-Paper CS
    constexpr int EPD_SCK = 7; // E-Paper SCK
    constexpr int EPD_SDI = 9; // E-Paper SDI  MOSI
    constexpr int EPD_RES = 38; // E-Paper RES
    constexpr int EPD_DC = 10; // E-Paper D/C
    // Battery voltage reading (TRMNL 7.5" OG DIY Kit)
    // Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
    constexpr int BATTERY_ADC = 1; // GPIO 1 (A0) - BAT_ADC (2:1 divider)
    constexpr int ADC_EN = 6; // GPIO 6 (A5) - ADC_EN (power control)
    // Button pins for temporary display mode switching
    constexpr int BUTTON_HALF_AND_HALF = 2; // GPIO 2 - Show DISPLAY_MODE_HALF_AND_HALF
    constexpr int BUTTON_WEATHER_ONLY = 3; // GPIO 3 - Show DISPLAY_MODE_WEATHER_ONLY
    constexpr int BUTTON_DEPARTURE_ONLY = 5; // GPIO 5 - Show DISPLAY_MODE_DEPARTURE_ONLY
#else
#error "Board not defined! Please specify board type in platformio.ini"
#endif
}
