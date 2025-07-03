#pragma once
#include <WebServer.h>
#include <vector>
#include "api/rmv_api.h"

void handleConfigPage(WebServer &server);
void handleConfigDone(WebServer &server);
void handleSaveConfig(WebServer &server);
void handleStopAutocomplete(WebServer &server);
void setupWebServer(WebServer &server);
