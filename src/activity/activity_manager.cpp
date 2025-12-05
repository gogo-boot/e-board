#include "activity/activity_manager.h"
#include "util/system_init.h"

static Lifecycle currentLifecycle = Lifecycle::ON_INIT;

Lifecycle ActivityManager::getCurrentActivityLifecycle() {
    return currentLifecycle;
}

void ActivityManager::setCurrentActivityLifecycle(Lifecycle status) {
    currentLifecycle = status;
}

void ActivityManager::onInit() {
    setCurrentActivityLifecycle(Lifecycle::ON_INIT);
    SystemInit::initialize();
}

void ActivityManager::onStart() {
    setCurrentActivityLifecycle(Lifecycle::ON_START);
}

void ActivityManager::onRunning() {
    setCurrentActivityLifecycle(Lifecycle::ON_RUNNING);
}

void ActivityManager::onStop() {
    setCurrentActivityLifecycle(Lifecycle::ON_STOP);
}

void ActivityManager::onShutdown() {
    setCurrentActivityLifecycle(Lifecycle::ON_SHUTDOWN);
}
