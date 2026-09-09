#include "capability_support.hpp"
#include <check.hpp>
#include <functional>
#include <type_traits>
#include <version>

int main() {
#if defined(__cpp_lib_copyable_function)
    print_macro("__cpp_lib_copyable_function", __cpp_lib_copyable_function);
#else
    return skip("__cpp_lib_copyable_function not defined");
#endif

#if !defined(__cpp_lib_copyable_function) || __cpp_lib_copyable_function < 202306L
    return skip("std::copyable_function unavailable");
#else
    static_assert(std::is_copy_constructible_v<std::copyable_function<int() const>>);
    std::copyable_function<int() const> f = [n = 41] { return n + 1; };
    auto copy = f;
    const auto& cf = copy;
    check(cf() == 42, "copyable_function copy and const invocation");
    return 0;
#endif
}

