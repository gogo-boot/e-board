#include "util/util.h"
#include <vector>
#include <set>
#include <map>

void Util::printFreeHeap(const char* msg)
{
    Serial.printf("%s Free heap: %u bytes\n", msg, ESP.getFreeHeap());
}

String Util::urlEncode(const String& str)
{
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else
        {
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

String Util::urlDecode(const String& str)
{
    String decoded = "";
    char temp[] = "00";
    unsigned int len = str.length();
    unsigned int i = 0;
    while (i < len)
    {
        char c = str.charAt(i);
        if (c == '%')
        {
            if (i + 2 < len)
            {
                temp[0] = str.charAt(i + 1);
                temp[1] = str.charAt(i + 2);
                decoded += (char)strtol(temp, nullptr, 16);
                i += 3;
            }
            else
            {
                decoded += c;
                i++;
            }
        }
        else if (c == '+')
        {
            decoded += ' ';
            i++;
        }
        else
        {
            decoded += c;
            i++;
        }
    }
    return decoded;
}

String Util::getUniqueSSID(const String& prefix)
{
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-%06X", prefix.c_str(), chipId & 0xFFFFFF);
    return String(ssid);
}


String Util::shortenDestination(const String departure, const String destination)
{
    // Tokenize departure
    std::vector<String> depTokens;
    int start = 0, end = 0;
    while ((end = departure.indexOf(' ', start)) != -1)
    {
        depTokens.push_back(departure.substring(start, end));
        start = end + 1;
    }
    depTokens.push_back(departure.substring(start));

    // Tokenize destination
    std::vector<String> destTokens;
    start = 0;
    while ((end = destination.indexOf(' ', start)) != -1)
    {
        destTokens.push_back(destination.substring(start, end));
        start = end + 1;
    }
    destTokens.push_back(destination.substring(start));

    // Remove matching tokens from the start
    size_t i = 0;
    while (i < depTokens.size() && i < destTokens.size() && depTokens[i] == destTokens[i])
    {
        ++i;
    }

    // Reconstruct string from remaining destination tokens
    String result = "";
    for (size_t j = i; j < destTokens.size(); ++j)
    {
        if (j > i) result += " ";
        result += destTokens[j];
    }

    // Map for replacements
    std::map<String, String> replacements = {
        {"Bahnhof", "Bhf"}, {"(Tanus)", "Ts"}, {"(Main)", "M"}, {"(Hbf)", "Hbf"}, {"(Hauptbahnhof)", "Hbf"}
    };

    // Replace map-matched replacements in result
    for (const auto& pair : replacements)
    {
        int idx = result.indexOf(pair.first);
        while (idx != -1)
        {
            result = result.substring(0, idx) + pair.second + result.substring(idx + pair.first.length());
            idx = result.indexOf(pair.first, idx + pair.second.length());
        }
    }
    return result;
}
