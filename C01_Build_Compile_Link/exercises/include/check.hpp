#pragma once
#include <cstdlib>
#include <iostream>
#include <string_view>

// A failed check is a process verdict, not an exception from the subject API.
// exit(1) also avoids an uncaught-exception dialog in the Windows Debug CRT.
inline void check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
