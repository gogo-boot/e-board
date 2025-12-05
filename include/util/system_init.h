#pragma once

namespace SystemInit {
    void initSerialConnector();
    void printWakeupCause();
    void factoryResetIfDesired();
    void initDisplay();
    void initFont();
    void loadNvsConfig();
}
