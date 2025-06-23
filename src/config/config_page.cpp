#include "config_page.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>

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
// Update the config page handler to serve config_my_station.html
void handleConfigPage(WebServer &server) {
  File file = LittleFS.open("/config_my_station.html", "r");
  if (!file) {
    server.send(404, "text/plain", "Konfigurationsseite nicht gefunden");
    return;
  }
  server.streamFile(file, "text/html; charset=utf-8");
  file.close();
}

// Handle the configuration done action (GET /done)
void handleConfigDone(WebServer &server, bool &inConfigMode) {
    inConfigMode = false;
    server.send(200, "text/html", "<h1>Configuration Complete</h1><p>Device will now run in normal mode.</p>");
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer &server,bool &inConfigMode) {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    // Parse JSON body
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    File f = LittleFS.open("/config.json", "w");
    if (!f) {
        server.send(500, "text/plain", "Failed to save config");
        return;
    }
    serializeJson(doc, f);
    f.close();
    inConfigMode = false;
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// AJAX handler to resolve station/stop name (GET /api/stop?q=...)
void handleStopAutocomplete(WebServer &server) {
    String query = server.hasArg("q") ? server.arg("q") : "";
    // TODO: Call RMV API and return JSON array of suggestions
    // For now, return dummy data
    DynamicJsonDocument doc(256);
    JsonArray arr = doc.to<JsonArray>();
    arr.add("Frankfurt Hauptbahnhof");
    arr.add("Frankfurt West");
    arr.add("Frankfurt Süd");
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}
