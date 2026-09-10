#include <format>
#include <iostream>
#include <system_error>

int main() {
#if !defined(__cpp_lib_format) || __cpp_lib_format < 201907L
    std::cout << "SKIP format is unavailable\n";
    return 77;
#elif !defined(C05_HAS_ERROR_CODE_FORMATTER)
    std::cout << "SKIP std::formatter<std::error_code, char> failed the configure-time semantic probe\n";
    return 77;
#else
    const std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    const std::string text   = std::format("{}", ec);
    if (text.empty()) {
        std::cerr << "FAIL error_code formatter produced empty text\n";
        return 1;
    }
    std::cout << "PASS error_code formatter text=" << text << '\n';
    return 0;
#endif
}
