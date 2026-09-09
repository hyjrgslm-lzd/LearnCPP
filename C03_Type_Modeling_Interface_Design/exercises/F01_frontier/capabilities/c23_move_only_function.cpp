#include "capability_support.hpp"
#include <check.hpp>
#include <functional>
#include <memory>
#include <type_traits>
#include <version>

int main() {
#if defined(__cpp_lib_move_only_function)
    print_macro("__cpp_lib_move_only_function", __cpp_lib_move_only_function);
#else
    return skip("__cpp_lib_move_only_function not defined");
#endif

#if !defined(__cpp_lib_move_only_function) || __cpp_lib_move_only_function < 202110L
    return skip("std::move_only_function unavailable");
#else
    static_assert(!std::is_copy_constructible_v<std::move_only_function<int()>>);
    auto state = std::make_unique<int>(40);
    std::move_only_function<int() const> f = [state = std::move(state)] { return *state + 2; };
    const auto& cf = f;
    check(cf() == 42, "const-qualified move_only_function invocation");
    auto moved = std::move(f);
    check(moved() == 42, "moved target still callable");
    return 0;
#endif
}

