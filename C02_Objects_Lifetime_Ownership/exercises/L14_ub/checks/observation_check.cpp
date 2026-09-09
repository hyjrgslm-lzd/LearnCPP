#include <check.hpp>

#include <iostream>
#include <string_view>

enum class BehaviorKind {
    undefined_behavior,
    ifndr,
    implementation_defined,
    unspecified,
    erroneous,
    well_defined,
};

constexpr std::string_view name(BehaviorKind kind)
{
    switch (kind) {
    case BehaviorKind::undefined_behavior: return "undefined_behavior";
    case BehaviorKind::ifndr: return "ifndr";
    case BehaviorKind::implementation_defined: return "implementation_defined";
    case BehaviorKind::unspecified: return "unspecified";
    case BehaviorKind::erroneous: return "erroneous";
    case BehaviorKind::well_defined: return "well_defined";
    }
    return "unknown";
}

int checked_read(const int* p)
{
    if (p == nullptr) {
        return 0;
    }
    return *p;
}

int main()
{
    int value = 5;
    check(checked_read(&value) == 5, "non-null read returns the value");
    check(checked_read(nullptr) == 0, "null is checked before dereference");

    std::cout << "dangling reference: " << name(BehaviorKind::undefined_behavior) << '\n';
    std::cout << "ODR no diagnostic case: " << name(BehaviorKind::ifndr) << '\n';
    std::cout << "plain char signedness: " << name(BehaviorKind::implementation_defined) << '\n';
    std::cout << "some evaluation orders: " << name(BehaviorKind::unspecified) << '\n';
    std::cout << "C++26 erroneous values: " << name(BehaviorKind::erroneous) << '\n';
    std::cout << "L14_ub_observation OK\n";
}
