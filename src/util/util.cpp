#include "util/util.h"

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

String Util::urlDecode(const String& str) {
    String decoded = "";
    char temp[] = "00";
    unsigned int len = str.length();
    unsigned int i = 0;
    while (i < len) {
        char c = str.charAt(i);
        if (c == '%') {
            if (i + 2 < len) {
                temp[0] = str.charAt(i + 1);
                temp[1] = str.charAt(i + 2);
                decoded += (char) strtol(temp, nullptr, 16);
                i += 3;
            } else {
                decoded += c;
                i++;
            }
        } else if (c == '+') {
            decoded += ' ';
            i++;
        } else {
            decoded += c;
            i++;
        }
    }
    return decoded;
}

String Util::getUniqueSSID(const String& prefix) {
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-%06X", prefix.c_str(), chipId & 0xFFFFFF);
    return String(ssid);
}
