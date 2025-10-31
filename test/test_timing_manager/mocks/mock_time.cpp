#include "mock_time.h"

// Initialize static members
time_t MockTime::mockCurrentTime = 0;
bool MockTime::useMock = false;

