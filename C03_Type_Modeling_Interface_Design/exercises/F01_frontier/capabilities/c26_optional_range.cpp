#include "capability_support.hpp"
#include <check.hpp>
#include <optional>
#include <version>

int main() {
#if defined(__cpp_lib_optional_range_support)
    print_macro("__cpp_lib_optional_range_support", __cpp_lib_optional_range_support);
#else
    return skip("__cpp_lib_optional_range_support not defined");
#endif

#if !defined(__cpp_lib_optional_range_support) || __cpp_lib_optional_range_support < 202406L
    return skip("std::optional range support unavailable");
#else
    std::optional<int> one = 4;
    int sum = 0;
    for (int x : one) {
        sum += x;
    }
    for (int x : std::optional<int>{}) {
        sum += x;
    }
    check(sum == 4, "optional iterates as zero-or-one range");
    return 0;
#endif
}

