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

Lifecycle ActivityManager::getCurrentActivityLifecycle() {
    return currentLifecycle;
}

void ActivityManager::setCurrentActivityLifecycle(Lifecycle status) {
    currentLifecycle = status;
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

    // Set up Time if it needed - To Move
    TimeManager::setupNTPTime();

    // Set temporary display mode if needed - To Move
    ButtonManager::handleWakeupMode();
}

void ActivityManager::onRunning() {
    setCurrentActivityLifecycle(Lifecycle::ON_RUNNING);

    // Start configuration Phase 2 if needed : Application Configuration
    //   - Todo Start WebServer if needed - To Move
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();
    if (phase == PHASE_APP_SETUP) {
        BootFlowManager::handlePhaseAppSetup();
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

    // Setup by pressing buttons can be woken up - To Move
    ButtonManager::setWakupableButtons();
}

void ActivityManager::onShutdown() {
    setCurrentActivityLifecycle(Lifecycle::ON_SHUTDOWN);
    // Turn off peripherals - To Move
    DisplayManager::hibernate();
    // Enter deep sleep mode - To Move
    enterDeepSleep(sleepTimeSeconds);
}
