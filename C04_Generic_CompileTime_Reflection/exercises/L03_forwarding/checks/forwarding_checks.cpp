#include <check.hpp>
#include <forwarding_tools.hpp>

#include <concepts>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace l03_checks {

struct CategoryProbe {
    int lvalue_calls{};
    int rvalue_calls{};

    int& operator()(int& value) noexcept {
        ++lvalue_calls;
        return value;
    }

    int operator()(int&& value) noexcept {
        ++rvalue_calls;
        return value + 10;
    }
};

struct Throwing {
    int operator()(int) {
        throw std::runtime_error("call failure");
    }
};

struct NoThrow {
    int operator()(int) noexcept {
        return 1;
    }
};

template<class Probe>
void check_lvalue_forwarding(Probe& probe, int& value) {
    if constexpr (std::same_as<decltype(l03::invoke_preserving(probe, value)), int&>) {
        int& result = l03::invoke_preserving(probe, value);
        check(&result == &value, "invoke_preserving returns lvalue reference from callable");
        result = 6;
        check(value == 6, "returned reference writes through original object");
    } else {
        (void)l03::invoke_preserving(probe, value);
        check(false, "invoke_preserving must keep lvalue argument category");
    }
    check(probe.lvalue_calls == 1 && probe.rvalue_calls == 0, "invoke_preserving must keep lvalue argument category");
}

void run_all() {
    CategoryProbe probe{};
    int value = 5;

    check_lvalue_forwarding(probe, value);

    int moved_result = l03::invoke_preserving(probe, 7);
    check(moved_result == 17, "invoke_preserving forwards rvalue argument category");
    check(probe.rvalue_calls == 1, "rvalue overload is selected once");

    check(noexcept(l03::invoke_preserving(NoThrow{}, 1)), "nothrow callable propagates noexcept");
    check(!noexcept(l03::invoke_preserving(Throwing{}, 1)), "throwing callable is not declared noexcept");

    bool threw = false;
    try {
        (void)l03::invoke_preserving(Throwing{}, 1);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "exception from callable propagates");

    l03::holder scalar{42};
    check((std::same_as<decltype(scalar), l03::holder<int>>), "CTAD deduces holder<int>");
    check(scalar.value == 42, "holder stores scalar value");

    auto text = l03::holder{std::string{"cpp"}};
    check((std::same_as<decltype(text), l03::holder<std::string>>), "CTAD deduces holder<string>");
    check(text.value == "cpp", "holder owns string value");

    auto list = l03::make_list_holder({1, 2, 3});
    check((std::same_as<decltype(list), l03::holder<std::vector<int>>>), "initializer_list entry returns holder<vector<int>>");
    check(list.value.size() == 3 && list.value[2] == 3, "initializer_list entry owns all elements");
}

} // namespace l03_checks

int main() {
    l03_checks::run_all();
    std::cout << "L03_forwarding checks OK\n";
}
