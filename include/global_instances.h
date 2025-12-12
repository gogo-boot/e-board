#pragma once

#include <WebServer.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

// Extern declarations - actual definitions are in main.cpp
// These allow all source files to access the same global instances

// Web server for configuration mode
extern WebServer server;

// E-Paper display for GDEY075T7 (800x480)
extern GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display;

// U8g2 font renderer for UTF-8 support (German umlauts)
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

// RTC memory for persistent state across deep sleep
extern RTC_DATA_ATTR unsigned long wakeupCount;


