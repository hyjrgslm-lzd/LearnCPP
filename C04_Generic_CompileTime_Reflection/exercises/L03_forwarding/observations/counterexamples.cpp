#include <check.hpp>

#include <iostream>
#include <type_traits>
#include <utility>

namespace l03_observation {

struct RefQualified {
    constexpr int call() & {
        return 1;
    }

    constexpr int call() && {
        return 2;
    }
};

template<class T>
constexpr int lost_forwarding(T&& object) {
    return object.call();
}

template<class T>
constexpr int kept_forwarding(T&& object) {
    return std::forward<T>(object).call();
}

template<class T>
struct ordinary_rvalue_holder {
    constexpr int accept(T&&) {
        return 1;
    }
};

} // namespace l03_observation

int main() {
    using namespace l03_observation;

    check(lost_forwarding(RefQualified{}) == 1, "named forwarding parameter is an lvalue inside the function");
    check(kept_forwarding(RefQualified{}) == 2, "std::forward restores rvalue category");

    static_assert(!std::is_invocable_v<decltype(&ordinary_rvalue_holder<int>::accept), ordinary_rvalue_holder<int>&, int&>);
    static_assert(std::is_invocable_v<decltype(&ordinary_rvalue_holder<int>::accept), ordinary_rvalue_holder<int>&, int&&>);

    std::cout << "L03_forwarding counterexamples observation OK\n";
}
