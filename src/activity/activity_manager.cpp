#include "activity/activity_manager.h"

#include "util/battery_manager.h"
#include "util/boot_flow_manager.h"
#include "util/button_manager.h"
#include "util/device_mode_manager.h"
#include "util/ota_manager.h"
#include "util/sleep_utils.h"
#include "util/system_init.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"

static Lifecycle currentLifecycle = Lifecycle::ON_INIT;
static const char* TAG = "ACTIVITY_MGR";

Lifecycle ActivityManager::getCurrentActivityLifecycle() {
    return currentLifecycle;
}

void ActivityManager::setCurrentActivityLifecycle(Lifecycle status) {
    ESP_LOGI(TAG, "Current Lifecycle : %s", lifecycleToString(status));
    currentLifecycle = status;
}

const char* ActivityManager::lifecycleToString(Lifecycle lifecycle) {
    switch (lifecycle) {
    case Lifecycle::ON_INIT: return "ON_INIT";
    case Lifecycle::ON_START: return "ON_START";
    case Lifecycle::ON_RUNNING: return "ON_RUNNING";
    case Lifecycle::ON_STOP: return "ON_STOP";
    case Lifecycle::ON_SHUTDOWN: return "ON_SHUTDOWN";
    default: return "UNKNOWN";
    }
}

void ActivityManager::onInit() {
    setCurrentActivityLifecycle(Lifecycle::ON_INIT);
    SystemInit::initSerialConnector();
    SystemInit::printWakeupCause();
    SystemInit::factoryResetIfDesired();
    SystemInit::initDisplay();
    SystemInit::initFont();
    BatteryManager::init();;
    SystemInit::loadNvsConfig();
}

void ActivityManager::onStart() {
    setCurrentActivityLifecycle(Lifecycle::ON_START);

    // Start configuration Phase 1 if needed : Wifi Manager Configuration
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();
    if (phase == PHASE_WIFI_SETUP) {
        BootFlowManager::handlePhaseWifiSetup();
    }
    // Setup by pressing buttons changes display mode while running - To make

    // Set up Time if it needed
    DeviceModeManager::setupConnectivityAndTime();

    // Set temporary display mode if needed
    ButtonManager::handleWakeupMode();
}

void ActivityManager::onRunning() {
    setCurrentActivityLifecycle(Lifecycle::ON_RUNNING);

    // Start configuration Phase 2 if needed : Application Configuration
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();
    if (phase == PHASE_APP_SETUP) {
        BootFlowManager::handlePhaseAppSetup();

        ConfigManager::setConfigMode(true);
        // STOP HERE - don't progress further
        // The web server will run in loop() for configuration
        return;
    }
    // OTA Update Check by checking scheduled time with RTC clock time
    OTAManager::checkAndApplyUpdate();

    // Fetch Data from APIs and Update Display
    if (phase == PHASE_COMPLETE) {
        BootFlowManager::handlePhaseComplete();
    }
}

uint64_t sleepTimeSeconds = 0;

void ActivityManager::onStop() {
    setCurrentActivityLifecycle(Lifecycle::ON_STOP);

    // Calculate next wake-up time - To Move
    sleepTimeSeconds = TimingManager::getNextSleepDurationSeconds();

    // clean up temporary states if needed - To make
    ConfigManager::setConfigMode(false);

    // Setup by pressing buttons can be woken up
    ButtonManager::setWakupableButtons();
}

void ActivityManager::onShutdown() {
    setCurrentActivityLifecycle(Lifecycle::ON_SHUTDOWN);

    // Turn off peripherals
    DisplayManager::hibernate();
    // Enter deep sleep mode
    enterDeepSleep(sleepTimeSeconds);
}
