#include <format>
#include <iostream>
#include <string>
#include <version>

int main() {
#if !defined(__cpp_lib_to_string) || __cpp_lib_to_string < 202306L
    std::cout << "SKIP __cpp_lib_to_string is missing or below 202306L\n";
    return 77;
#elif !defined(__cpp_lib_format) || __cpp_lib_format < 201907L
    std::cout << "SKIP format unavailable; cannot compare C++26 to_string wording\n";
    return 77;
#else
    const std::string one       = std::to_string(1.0);
    const std::string formatted = std::format("{}", 1.0);
    if (one != formatted || one == "1.000000") {
        std::cerr << "FAIL to_string(double) did not follow format-style output\n";
        return 1;
    }

    const std::string precise = std::to_string(1.25);
    if (precise != std::format("{}", 1.25)) {
        std::cerr << "FAIL to_string precision differs from format\n";
        return 1;
    }

    std::cout << "PASS to_string semantics value=" << one << '\n';
    return 0;
#endif
}
