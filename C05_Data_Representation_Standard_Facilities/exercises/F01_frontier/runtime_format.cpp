#include <format>
#include <iostream>
#include <string>
#include <version>

int main() {
#if !defined(__cpp_lib_format) || __cpp_lib_format < 202311L
    std::cout << "SKIP __cpp_lib_format is below 202311L; runtime_format not declared\n";
    return 77;
#else
    const std::string pattern = "{} + {} = {}";
    const std::string out     = std::format(std::runtime_format(pattern), 2, 3, 5);
    if (out != "2 + 3 = 5") {
        std::cerr << "FAIL runtime_format output\n";
        return 1;
    }

    int lhs                   = 2;
    int rhs                   = 3;
    int sum                   = 5;
    const auto args           = std::make_format_args(lhs, rhs, sum);
    const std::string via_c23 = std::vformat(pattern, args);
    if (via_c23 != out) {
        std::cerr << "FAIL vformat comparison\n";
        return 1;
    }

    std::cout << "PASS runtime_format\n";
    return 0;
#endif
}
