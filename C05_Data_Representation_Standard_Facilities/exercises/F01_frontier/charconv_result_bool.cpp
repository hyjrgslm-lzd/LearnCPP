#include <charconv>
#include <iostream>
#include <system_error>
#include <version>

int main() {
#if !defined(__cpp_lib_to_chars) || __cpp_lib_to_chars < 202306L
    std::cout << "SKIP __cpp_lib_to_chars is below 202306L; charconv result bool not declared\n";
    return 77;
#else
    int value = 0;
    const char* first = "42";
    const char* last  = first + 2;
    auto parsed       = std::from_chars(first, last, value);
    if (!static_cast<bool>(parsed) || parsed.ptr != last || value != 42) {
        std::cerr << "FAIL from_chars_result bool success path\n";
        return 1;
    }

    auto rejected = std::from_chars(first, first, value);
    if (static_cast<bool>(rejected) || rejected.ec != std::errc::invalid_argument) {
        std::cerr << "FAIL from_chars_result bool failure path\n";
        return 1;
    }

    char out[8]{};
    auto written = std::to_chars(out, out + 8, 42);
    if (!static_cast<bool>(written) || written.ptr != out + 2 || out[0] != '4' || out[1] != '2') {
        std::cerr << "FAIL to_chars_result bool success path\n";
        return 1;
    }

    std::cout << "PASS charconv result bool\n";
    return 0;
#endif
}
