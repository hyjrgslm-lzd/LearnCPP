#include "capability_support.hpp"
#include <check.hpp>
#include <string>
#include <variant>
#include <version>

int main() {
#if defined(__cpp_lib_variant)
    print_macro("__cpp_lib_variant", __cpp_lib_variant);
#else
    return skip("__cpp_lib_variant not defined");
#endif

#if !defined(__cpp_lib_variant) || __cpp_lib_variant < 202306L
    return skip("std::variant member visit unavailable");
#else
    std::variant<int, std::string> value = std::string{"abc"};
    auto size = value.visit([](const auto& item) -> int {
        if constexpr (std::is_same_v<std::decay_t<decltype(item)>, std::string>) {
            return static_cast<int>(item.size());
        } else {
            return item;
        }
    });
    check(size == 3, "variant member visit forwards to visitor");
    return 0;
#endif
}

