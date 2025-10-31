#ifndef MOCK_TIME_H
#define MOCK_TIME_H

#include <ctime>

class MockTime {
private:
    static time_t mockCurrentTime;
    static bool useMock;

public:
    // Set a specific time for testing
    static void setMockTime(time_t time) {
        mockCurrentTime = time;
        useMock = true;
    }

    // Reset to use real system time
    static void useRealTime() {
        useMock = false;
    }

    // Get current time (mocked or real)
    static time_t now() {
        if (useMock) {
            return mockCurrentTime;
        }
        return time(nullptr);
    }
};


#endif // MOCK_TIME_H

