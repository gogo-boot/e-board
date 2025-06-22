#include "config_page.h"
#include <LittleFS.h>

void handleStationSelect(WebServer &server, const std::vector<Station>& stations) {
    File file = LittleFS.open("/station_select.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Template not found");
        return;
    }
    String page = file.readString();
    file.close();
    String html;
    for (size_t i = 0; i < stations.size(); ++i) {
        const auto& s = stations[i];
        html += "<div class='station'>";
        html += "<input type='radio' name='station' value='" + s.id + "'>";
        html += s.name + " (" + s.type + ")";
        html += "</div>";
    }
    page.replace("{{stations}}", html);
    server.send(200, "text/html", page);
}

void handleConfigPage(WebServer &server) {
    server.send(200, "text/html", "<h1>ESP32 Configuration Page</h1><p>Put your config form here.</p>");
}

void handleConfigDone(WebServer &server, bool &inConfigMode) {
    inConfigMode = false;
    server.send(200, "text/html", "<h1>Configuration Complete</h1><p>Device will now run in normal mode.</p>");
}
