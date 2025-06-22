#include "util.h"

void Util::printFreeHeap(const char* msg) {
    Serial.printf("%s Free heap: %u bytes\n", msg, ESP.getFreeHeap());
}

String Util::urlEncode(const String& str) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            code0 = ((c >> 4) & 0xf) + '0';
            if (((c >> 4) & 0xf) > 9) code0 = ((c >> 4) & 0xf) - 10 + 'A';
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

String Util::getUniqueSSID(const String& prefix) {
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-%06X", prefix.c_str(), chipId & 0xFFFFFF);
    return String(ssid);
}
