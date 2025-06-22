#pragma once
#include <Arduino.h>

String buildWifiJson();
bool getLocationFromGoogle(float &lat, float &lon);
