#include "capability_support.hpp"
#include <check.hpp>
#include <version>

#if __has_include(<contracts>)
#include <contracts>
#define F01_HAS_CONTRACTS_HEADER 1
#else
#define F01_HAS_CONTRACTS_HEADER 0
#endif

#if defined(__cpp_contracts)
int positive(int value)
    pre(value > 0)
    post(result: result > value)
{
    contract_assert(value < 100);
    return value + 1;
}
#endif

int main() {
#if defined(__cpp_contracts)
    print_macro("__cpp_contracts", __cpp_contracts);
#else
    return skip("__cpp_contracts not defined");
#endif

#if defined(__cpp_lib_contracts)
    print_macro("__cpp_lib_contracts", __cpp_lib_contracts);
#else
    std::cout << "__cpp_lib_contracts=not-defined\n";
#endif
    print_macro("has_contracts_header", F01_HAS_CONTRACTS_HEADER);

#if defined(__cpp_contracts)
    check(positive(41) == 42, "contracts syntax compiled and legal call path ran");
    return 0;
#endif
}
