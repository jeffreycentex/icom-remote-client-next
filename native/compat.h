// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Desktop transmit timing is unused by the bundled receive-only adapter.
// The generated CW source also includes this header on MinGW.
#if defined(__cplusplus) && defined(_WIN32)
#include <ctime>

inline unsigned int timeBeginPeriod(unsigned int) { return 0; }
inline unsigned int timeEndPeriod(unsigned int) { return 0; }
inline std::tm* gmtime_r(const std::time_t* value, std::tm* result) {
    return ::gmtime_s(result, value) == 0 ? result : nullptr;
}
inline std::tm* localtime_r(const std::time_t* value, std::tm* result) {
    return ::localtime_s(result, value) == 0 ? result : nullptr;
}
#endif
