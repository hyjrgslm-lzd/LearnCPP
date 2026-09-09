#include <check.hpp>
#include <string>
#include <type_traits>
#include <utility>

namespace {

struct Item {
    int value;
};

Item make_item() {
    return Item{7};
}

int& as_lvalue(Item& item) {
    return item.value;
}

int&& as_xvalue(Item& item) {
    return std::move(item.value);
}

void accept(int&) {}
void accept(int const&) {}
void accept(int&&) {}

template <class T>
constexpr bool is_lvalue_param(T&& value) {
    (void)value;
    return std::is_lvalue_reference_v<T>;
}

template <class T>
decltype(auto) identity_forward(T&& value) {
    return std::forward<T>(value);
}

} // namespace

int main() {
    Item item{3};
    static_assert(std::is_lvalue_reference_v<decltype((item))>);
    static_assert(!std::is_reference_v<decltype(item)>);
    static_assert(std::is_same_v<decltype(make_item()), Item>);
    static_assert(std::is_rvalue_reference_v<decltype(as_xvalue(item))>);
    static_assert(std::is_lvalue_reference_v<decltype(as_lvalue(item))>);

    accept(item.value);
    accept(Item{4}.value);
    accept(as_xvalue(item));

    check(is_lvalue_param(item), "forwarding reference sees lvalue argument");
    check(!is_lvalue_param(Item{9}), "forwarding reference sees prvalue argument");

    int value = 11;
    decltype(auto) ref = identity_forward(value);
    ref = 12;
    check(value == 12, "decltype(auto) can preserve lvalue reference");

    auto copied = identity_forward(value);
    copied = 13;
    check(value == 12, "plain auto drops reference in variable declaration");

    auto materialized = make_item();
    check(materialized.value == 7, "prvalue materializes into destination object");
}
