#include "capability_support.hpp"
#include <check.hpp>
#include <expected>
#include <string>
#include <version>

int main() {
#if defined(__cpp_lib_expected)
    print_macro("__cpp_lib_expected", __cpp_lib_expected);
#else
    return skip("__cpp_lib_expected not defined");
#endif

#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202211L
    return skip("std::expected unavailable");
#else
    auto half = [](int x) -> std::expected<int, std::string> {
        if (x % 2 == 0) {
            return x / 2;
        }
        return std::unexpected{"odd"};
    };
    auto ok = std::expected<int, std::string>{84}
        .and_then(half)
        .transform([](int x) { return x + 1; });
    check(ok && *ok == 43, "expected value channel");
    auto err = std::expected<int, std::string>{std::unexpected{"bad"}}
        .or_else([](const std::string& e) {
            return std::expected<int, std::string>{std::unexpected{"wrapped:" + e}};
        })
        .transform_error([](const std::string& e) { return e + "!"; });
    check(!err && err.error() == "wrapped:bad!", "expected error channel");
    return 0;
#endif
}

