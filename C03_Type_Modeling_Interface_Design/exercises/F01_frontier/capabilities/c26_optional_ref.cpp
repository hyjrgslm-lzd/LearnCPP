#include "capability_support.hpp"
#include <check.hpp>
#include <optional>
#include <version>

int main() {
#if defined(__cpp_lib_optional)
    print_macro("__cpp_lib_optional", __cpp_lib_optional);
#else
    return skip("__cpp_lib_optional not defined");
#endif

#if !defined(__cpp_lib_optional) || __cpp_lib_optional < 202506L
    return skip("std::optional<T&> unavailable");
#else
    int a = 1;
    int b = 2;
    std::optional<int&> ref = a;
    check(ref && &*ref == &a, "optional reference binds lvalue");
    ref = b;
    check(&*ref == &b && a == 1, "optional reference assignment rebinds");
    *ref = 5;
    check(b == 5, "optional reference writes referent");
    return 0;
#endif
}

