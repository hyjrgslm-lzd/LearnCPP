#include <check.hpp>

#include <concepts>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c04_l11_counterexamples {

struct RefBox {
    int value{1};

    int& read_value() & noexcept { return value; }
};

template<class T>
auto loses_reference(T&& object) {
    return std::forward<T>(object).read_value();
}

struct RefQualifiedBox {
    int value{2};

    int& read_value() & noexcept { return value; }
    int&& read_value() && noexcept { return std::move(value); }
};

template<class T>
decltype(auto) forgets_forward(T&& object) noexcept(noexcept(object.read_value())) {
    return object.read_value();
}

struct ThrowingBox {
    int value{3};

    int& read_value() & {
        throw std::runtime_error("throws");
    }
};

template<class T>
decltype(auto) hardcoded_noexcept(T&& object) noexcept {
    return std::forward<T>(object).read_value();
}

namespace polluted_adl {

struct NoAdlBox {
    int value{4};
};

inline int recursion_depth = 0;

struct read_value_fn {
    template<class T>
    int operator()(T&& object) const;
};

inline constexpr read_value_fn read_value{};

template<class T>
int read_value_fn::operator()(T&& object) const {
    if (++recursion_depth > 1) {
        --recursion_depth;
        return -999;
    }
    int result = read_value(std::forward<T>(object));
    --recursion_depth;
    return result;
}

template<class T>
concept accepted_by_polluted_lookup = requires(T&& object) {
    read_value(std::forward<T>(object));
};

} // namespace polluted_adl

} // namespace c04_l11_counterexamples

int main() {
    using namespace c04_l11_counterexamples;

    RefBox ref{};
    auto copied = loses_reference(ref);
    copied = 7;
    check(ref.value == 1, "auto return copied the member value instead of preserving int&");
    check(!std::same_as<decltype(loses_reference(ref)), int&>, "auto return type is not int&");

    using forwarded_rvalue_result = decltype(std::declval<RefQualifiedBox&&>().read_value());
    using no_forward_result = decltype(forgets_forward(std::declval<RefQualifiedBox&&>()));
    check((std::same_as<forwarded_rvalue_result, int&&>), "direct rvalue member call returns int&&");
    check((std::same_as<no_forward_result, int&>), "missing std::forward turns the parameter into an lvalue call");

    ThrowingBox throwing{};
    check(noexcept(hardcoded_noexcept(throwing)), "hardcoded noexcept claims a throwing member cannot throw");
    check(!noexcept(throwing.read_value()), "the underlying member is actually throwing");

    using polluted_adl::NoAdlBox;
    static_assert(polluted_adl::accepted_by_polluted_lookup<NoAdlBox&>);
    NoAdlBox no_adl{};
    int polluted_result = polluted_adl::read_value(no_adl);
    check(polluted_result == -999, "unisolated ADL lookup accepted a no-path object and recursed into the CPO");

    std::cout << "L11_customization counterexamples observation OK\n";
}
