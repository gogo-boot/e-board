#include "activity/activity_manager.h"

Lifecycle ActivityManager::currentLifecycle = Lifecycle::ON_INIT;

Lifecycle ActivityManager::getCurrentActivityLifecycle() {
    return currentLifecycle;
}

void ActivityManager::setCurrentActivityLifecycle(Lifecycle status) {
    currentLifecycle = status;
}

void ActivityManager::onInit() {
    setCurrentActivityLifecycle(Lifecycle::ON_INIT);
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

}
void ActivityManager::onShutdown() {
    setCurrentActivityLifecycle(Lifecycle::ON_SHUTDOWN);
}
