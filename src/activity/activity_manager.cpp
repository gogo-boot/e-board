#include "activity/activity_manager.h"

#include "build_config.h"
#include "util/battery_manager.h"
#include "util/device_mode_manager.h"
#include "util/button_manager.h"
#include "util/device_mode_manager.h"
#include "ota/ota_manager.h"
#include "util/sleep_utils.h"
#include "util/system_init.h"
#include "util/timing_manager.h"
#include "config/config_manager.h"
#include "display/display_manager.h"
#include "util/wifi_manager.h"

static Lifecycle currentLifecycle = Lifecycle::ON_INIT;
static Lifecycle nextLifecycle = Lifecycle::ON_START;
static const char* TAG = "ACTIVITY_MGR";

Lifecycle ActivityManager::getCurrentActivityLifecycle() {
    return currentLifecycle;
}

void ActivityManager::setCurrentActivityLifecycle(Lifecycle status) {
    ESP_LOGI(TAG, "Current Lifecycle : %s", lifecycleToString(status));
    currentLifecycle = status;
}

Lifecycle ActivityManager::getNextActivityLifecycle() {
    return nextLifecycle;
}

void ActivityManager::setNextActivityLifecycle(Lifecycle status) {
    ESP_LOGI(TAG, "Current Lifecycle : %s", lifecycleToString(status));
    nextLifecycle = status;
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
    DEBUG_ONLY(SystemInit::initSerialConnector(););
    printWakeupReason();
    SystemInit::handleButtonLongPressActions();
    SystemInit::initDisplay();
    SystemInit::initFont();
    BatteryManager::init();
#if SHOW_BATTERY_STATUS
    float batteryVoltage = BatteryManager::getBatteryVoltage();
    // 0V means no battery connected (USB-only power) — skip the check
    if (batteryVoltage > 0.1f && batteryVoltage <= BATTERY_VOLTAGE_MIN) {
        DisplayManager::displayErrorIfBatteryLow();
        // Shutdown immediately if battery is low
        setNextActivityLifecycle(Lifecycle::ON_SHUTDOWN);
        return;
    }
#endif
    SystemInit::loadNvsConfig();

    // Attach GPIO interrupts so button presses are detected while awake
    ButtonManager::setWakupableButtons();
    ButtonManager::attachRunningInterrupts();

    DEBUG_ONLY(ConfigManager::printConfiguration(false);)

    setNextActivityLifecycle(Lifecycle::ON_START);
}

void ActivityManager::onStart() {
    setCurrentActivityLifecycle(Lifecycle::ON_START);

    // Start configuration Phase 1 if needed : Wifi Manager Configuration
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();
    if (phase == PHASE_WIFI_SETUP) {
        DeviceModeManager::handlePhaseWifiSetup();
    }

    // Start Wifi connection. If gets failed, show Wifi Error Screen
    //
    // Performance optimization: a day-browse button wake in Weather-Only mode
    // renders entirely from the RTC cache (weather + day hourly data), so WiFi
    // is unnecessary unless a weather refresh is actually due. Skipping the
    // connect saves ~1-2s of radio-on time and its power per button press.
    RTCConfigData& cfg = ConfigManager::getConfig();
    int8_t wakeButtonMode = ButtonManager::getWakeupButtonMode();
    bool isDayBrowseButtonWake =
        (wakeButtonMode >= 0) &&
        (cfg.displayMode == DISPLAY_MODE_WEATHER_ONLY) &&
        (wakeButtonMode != DISPLAY_MODE_APPLICATION_INFO);
    bool skipWifi = isDayBrowseButtonWake
        && !TimingManager::isTimeForWeatherUpdate()
        && !OTAManager::shouldCheckForUpdate();

    if (skipWifi) {
        ESP_LOGI(TAG, "Day-browse button wake, no weather update due — skipping WiFi (render from RTC cache)");
        ButtonManager::handleWakeupMode();
        setNextActivityLifecycle(Lifecycle::ON_RUNNING);
        return;
    }

    MyWiFiManager::reconnectWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        DisplayManager::displayErrorIfWifiConnectionError();
        setNextActivityLifecycle(Lifecycle::ON_STOP);
        return;
    }

    // Check if button was pressed during WiFi connect
    ButtonManager::checkAndRestartIfButtonPressed();

    // Set up Time if it needed
    DeviceModeManager::setupConnectivityAndTime();

    // Set temporary display mode (from short-press EXT1 wakeup or long-press synthetic mode)
    ButtonManager::handleWakeupMode();

    setNextActivityLifecycle(Lifecycle::ON_RUNNING);
}

void ActivityManager::onRunning() {
    setCurrentActivityLifecycle(Lifecycle::ON_RUNNING);

    // Start configuration Phase 2 if needed : Application Configuration
    ConfigPhase phase = DeviceModeManager::getCurrentPhase();
    if (phase == PHASE_APP_SETUP) {
        DeviceModeManager::handlePhaseAppSetup();
        setNextActivityLifecycle(Lifecycle::ON_LOOP);
        // The web server will run in loop() for configuration
        return;
    }

    // Check if button was pressed before OTA/API work
    ButtonManager::checkAndRestartIfButtonPressed();

    // OTA Update Check by checking scheduled time with RTC clock time
    if (OTAManager::checkAndApplyUpdate()) {
        // "Up to date" screen is shown — skip normal rendering, sleep 2 min
        setNextActivityLifecycle(Lifecycle::ON_STOP);
        return;
    }

    // Check if button was pressed during OTA check
    ButtonManager::checkAndRestartIfButtonPressed();

    // Fetch Data from APIs and Update Display
    if (phase == PHASE_COMPLETE) {
        DeviceModeManager::handlePhaseComplete();
    }

    // Check if button was pressed during API fetch / display render
    ButtonManager::checkAndRestartIfButtonPressed();

    setNextActivityLifecycle(Lifecycle::ON_STOP);
}

static uint64_t sleepTimeSeconds = 0;

void ActivityManager::onStop() {
    setCurrentActivityLifecycle(Lifecycle::ON_STOP);

    // Calculate next wake-up time - To Move
    sleepTimeSeconds = TimingManager::getNextSleepDurationSeconds();

    // Setup by pressing buttons can be woken up
    ButtonManager::setWakupableButtons();

    setNextActivityLifecycle(Lifecycle::ON_SHUTDOWN);
}

void ActivityManager::onShutdown() {
    setCurrentActivityLifecycle(Lifecycle::ON_SHUTDOWN);

    // Final check: if button was pressed while awake, restart to handle it
    ButtonManager::checkAndRestartIfButtonPressed();

    // Turn off peripherals
    DisplayManager::hibernate();

    // Use minimum sleep time if none was calculated (e.g. battery-low shutdown)
    if (sleepTimeSeconds == 0) {
        sleepTimeSeconds = 3600; // 1 hour fallback
    }

    // Enter deep sleep mode
    enterDeepSleep(sleepTimeSeconds);
}
