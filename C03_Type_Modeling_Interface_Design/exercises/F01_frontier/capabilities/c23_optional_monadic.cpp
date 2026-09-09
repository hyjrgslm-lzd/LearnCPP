#include "capability_support.hpp"
#include <check.hpp>
#include <optional>
#include <string>
#include <version>

int main() {
#if defined(__cpp_lib_optional)
    print_macro("__cpp_lib_optional", __cpp_lib_optional);
#else
    return skip("__cpp_lib_optional not defined");
#endif

#if !defined(__cpp_lib_optional) || __cpp_lib_optional < 202110L
    return skip("std::optional monadic operations unavailable");
#else
    auto parse = [](std::string text) -> std::optional<int> {
        if (text == "42") {
            return 42;
        }
        return std::nullopt;
    };
    auto value = std::optional<std::string>{"42"}
        .and_then(parse)
        .transform([](int x) { return x + 1; })
        .or_else([] { return std::optional<int>{7}; });
    check(value && *value == 43, "optional monadic value path");
    auto fallback = std::optional<std::string>{}
        .and_then(parse)
        .transform([](int x) { return x + 1; })
        .or_else([] { return std::optional<int>{7}; });
    check(fallback && *fallback == 7, "optional monadic empty path");
    return 0;
#endif
}

