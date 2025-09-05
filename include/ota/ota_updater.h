#pragma once
#include <Update.h>

class OTAUpdater {
public:
    struct FirmwareInfo {
        String version;
        String downloadUrl;
        int size;
        bool available;
    };

    static bool checkForUpdate(FirmwareInfo& info);
    static bool performUpdate(const String& url);
    static String getCurrentVersion();

private:
    static bool downloadAndInstall(const String& url);
    static void onProgress(int progress, int total);
};
