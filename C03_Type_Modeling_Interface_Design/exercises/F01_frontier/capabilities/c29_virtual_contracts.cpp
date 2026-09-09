#include "capability_support.hpp"
#include <check.hpp>
#include <version>

#if defined(__cpp_contracts) && __cpp_contracts >= 202606L
struct Base {
    virtual ~Base() = default;
    virtual int f(int value)
        pre(value > 0)
    {
        return value + 1;
    }
};

struct Derived : Base {
    int f(int value) override
        pre(value < 100)
    {
        return value + 2;
    }
};
#endif

int main() {
#if defined(__cpp_contracts)
    print_macro("__cpp_contracts", __cpp_contracts);
#else
    return skip("__cpp_contracts not defined");
#endif

#if !defined(__cpp_contracts) || __cpp_contracts < 202606L
    return skip("C++29 virtual contracts unavailable; SD-6 records 202606 for P3097R3");
#else
    Derived derived;
    Base& base = derived;
    check(base.f(40) == 42, "virtual contract syntax compiled and dispatched on legal path");
    return 0;
#endif
}
