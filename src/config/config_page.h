#pragma once
#include <WebServer.h>
#include <vector>
#include "api/rmv_api.h"

void handleConfigPage(WebServer &server);
void handleConfigDone(WebServer &server, bool &inConfigMode);
void handleSaveConfig(WebServer &server, bool &inConfigMode);
void handleStopAutocomplete(WebServer &server);
