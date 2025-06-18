#include <Arduino.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "secrets.h"


void setup() {
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // put your setup code here, to run once:
  Serial.begin(115200);
  
  Serial.println("Hello, PlatformIO World!");
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // Only show wifi (removes exit, info, update buttons)
  const char* menu[] = { "wifi" };
  wm.setMenu(menu, 1);

  //set custom ip for portal
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("MyStation"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }
}

void scanAccessPoints() {
  int n = WiFi.scanNetworks();
  Serial.printf("Found %d networks:\n", n);
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    String bssid = WiFi.BSSIDstr(i); // MAC address
    int rssi = WiFi.RSSI(i);
    Serial.printf("SSID: %s, BSSID: %s, RSSI: %d\n", ssid.c_str(), bssid.c_str(), rssi);
  }
  WiFi.scanDelete(); // Free memory
}

String buildWifiJson() {
  StaticJsonDocument<1024> doc;
  doc["considerIp"] = false;
  JsonArray aps = doc.createNestedArray("wifiAccessPoints");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    JsonObject ap = aps.createNestedObject();
    ap["macAddress"] = WiFi.BSSIDstr(i);
    ap["signalStrength"] = WiFi.RSSI(i);
    ap["signalToNoiseRatio"] = 0;
  }
  WiFi.scanDelete();
  String output;
  serializeJson(doc, output);
  return output;
}

void getLocationFromGoogle(const String& apiKey) {
  String wifiJson = buildWifiJson();
  HTTPClient http;
  String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + apiKey;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(wifiJson);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      float lat = doc["location"]["lat"];
      float lon = doc["location"]["lng"];
      Serial.printf("Google Location: Latitude: %f, Longitude: %f\n", lat, lon);
    } else {
      Serial.println("Failed to parse Google JSON response");
    }
  } else {
    Serial.printf("Google HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    getLocationFromGoogle(GOOGLE_API_KEY);
  } else {
    Serial.println("WiFi not connected");
  }
  delay(60000); // Wait 60 seconds before next request
}
