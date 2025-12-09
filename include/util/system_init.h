#pragma once

namespace SystemInit {
    void initSerialConnector();
    void factoryResetIfDesired();
    void initDisplay();
    void initFont();
    void loadNvsConfig();
}
