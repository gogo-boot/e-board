#pragma once
#include <Arduino.h>

enum class Lifecycle {
    ON_INIT,
    ON_START,
    ON_RUNNING,
    ON_STOP,
    ON_SHUTDOWN
};

class ActivityManager {
public:
    static Lifecycle getCurrentActivityLifecycle();

    static void onInit();
    static void onStart();
    static void onRunning();
    static void onStop();
    static void onShutdown();

private:
    static void setCurrentActivityLifecycle(Lifecycle status);
};
