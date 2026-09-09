#include "capability_support.hpp"
#include <check.hpp>
#include <expected>
#include <optional>
#include <vector>
#include <version>

int main() {
#ifdef _MSVC_STL_HARDENING
    print_macro("_MSVC_STL_HARDENING", _MSVC_STL_HARDENING);
#else
    std::cout << "_MSVC_STL_HARDENING=not-defined\n";
#endif

#ifdef __cpp_lib_contracts
    print_macro("__cpp_lib_contracts", __cpp_lib_contracts);
#else
    std::cout << "__cpp_lib_contracts=not-defined\n";
#endif

    std::vector<int> values{1, 2, 3};
    std::optional<int> maybe = values.at(1);
    std::expected<int, const char*> result = maybe ? std::expected<int, const char*>{*maybe}
                                                   : std::unexpected{"empty"};
    check(result && *result == 2, "hardening info probe avoids violating preconditions");
    return 0;
}

