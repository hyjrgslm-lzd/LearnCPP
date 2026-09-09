#include "capability_support.hpp"
#include <check.hpp>
#include <functional>
#include <type_traits>
#include <version>

int main() {
#if defined(__cpp_lib_function_ref)
    print_macro("__cpp_lib_function_ref", __cpp_lib_function_ref);
#else
    return skip("__cpp_lib_function_ref not defined");
#endif

#if !defined(__cpp_lib_function_ref) || __cpp_lib_function_ref < 202604L
    return skip("std::function_ref unavailable");
#else
    int base = 40;
    auto add = [&](int x) { return base + x; };
    std::function_ref<int(int)> ref = add;
    static_assert(!std::is_default_constructible_v<std::function_ref<int(int)>>);
    check(ref(2) == 42, "function_ref invokes referenced callable");
    base = 41;
    check(ref(1) == 42, "function_ref observes referenced callable state");
    return 0;
#endif
}

