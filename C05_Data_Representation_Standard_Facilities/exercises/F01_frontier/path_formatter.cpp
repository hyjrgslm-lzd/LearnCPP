#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <version>

int main() {
#if !defined(__cpp_lib_format_path) || __cpp_lib_format_path < 202403L
    std::cout << "SKIP __cpp_lib_format_path is missing or below 202403L\n";
    return 77;
#else
    const std::filesystem::path plain{"alpha/beta.txt"};
    const std::string displayed = std::format("{}", plain);
    const std::string generic   = std::format("{:g}", plain);
    if (displayed.empty() || generic.find("alpha") == std::string::npos || generic.find('/') == std::string::npos) {
        std::cerr << "FAIL path formatter basic/generic output\n";
        return 1;
    }

    const std::filesystem::path with_newline{"multi\nline"};
    const std::string escaped = std::format("{:?}", with_newline);
    if (escaped.find("\\n") == std::string::npos) {
        std::cerr << "FAIL path formatter debug escaping\n";
        return 1;
    }

#if __cpp_lib_format_path >= 202506L
    const std::string display_api = with_newline.display_string();
    const std::string system_api  = with_newline.system_encoded_string();
    if (display_api != std::format("{}", with_newline) || system_api.empty()) {
        std::cerr << "FAIL path display/system encoded observers\n";
        return 1;
    }
#endif

    std::cout << "PASS path formatter displayed=" << displayed << '\n';
    return 0;
#endif
}
