#include <explicit_object.hpp>

#include <check.hpp>

#include <concepts>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

namespace {
using c04_explicit::chain;
using c04_explicit::name_holder;
using c04_explicit::slot;

template<class T>
concept can_take_const_rvalue = requires(T const value) {
    std::move(value).take();
};
}

int main() {
    slot<int> value{7};
    decltype(auto) lref = value.value();
    check((std::same_as<decltype(lref), int&>) && &lref == value.address(),
        "value preserves object identity and cvref category");
    lref = 8;
    check(*value.address() == 8, "mutable lvalue writes through");

    const slot<int> const_value{9};
    decltype(auto) cref = const_value.value();
    check((std::same_as<decltype(cref), const int&>) && &cref == const_value.address(),
        "const lvalue returns const reference");

    decltype(auto) rref = std::move(value).value();
    check((std::same_as<decltype(rref), int&&>) && &rref == value.address(),
        "mutable rvalue forwards as rvalue reference");

    decltype(auto) crref = std::move(const_value).value();
    check((std::same_as<decltype(crref), const int&&>) && &crref == const_value.address(),
        "const rvalue forwards as const rvalue reference");

    check(noexcept(value.value()) && noexcept(std::move(value).value()),
        "value keeps noexcept on all cvref paths");

    slot<std::unique_ptr<int>> owner{std::make_unique<int>(42)};
    auto moved = std::move(owner).take();
    check(moved && *moved == 42 && owner.empty(), "move-only value can be consumed from mutable rvalue");
    check(!can_take_const_rvalue<slot<std::unique_ptr<int>>>, "const rvalue cannot move a move-only value");

    auto list = std::make_unique<chain>(1);
    list->next = std::make_unique<chain>(2);
    list->next->next = std::make_unique<chain>(3);
    check(list->sum() == 6, "recursive explicit-object lambda walks the chain");

    name_holder names{"outer"};
    decltype(auto) name = names.name();
    check((std::same_as<decltype(name), std::string&>) && &name == names.address(),
        "external borrow is a reference to the stored object");
    check(noexcept(names.name()), "borrow accessor is noexcept");

    std::cout << "A03 explicit object contract passed\n";
}
