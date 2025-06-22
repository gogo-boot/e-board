#pragma once
#include <WebServer.h>
#include <vector>
#include <Arduino.h>
#include "../api/rmv_api.h"

void handleStationSelect(WebServer &server, const std::vector<Station>& stations);
void handleConfigPage(WebServer &server);
void handleConfigDone(WebServer &server, bool &inConfigMode);
