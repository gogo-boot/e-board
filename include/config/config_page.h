#pragma once
#include <WebServer.h>

void handleConfigPage(WebServer& server);
void handleSaveConfig(WebServer& server);
void handleStopAutocomplete(WebServer& server);
void setupWebServer(WebServer& server);
