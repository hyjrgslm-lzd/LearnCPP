#include <explicit_object.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

using c04_explicit::name_holder;
using c04_explicit::slot;

template<class T>
concept can_take_lvalue = requires(T value) {
    value.take();
};

template<class T>
concept can_take_const_lvalue = requires(T const value) {
    value.take();
};

template<class T>
concept can_take_const_rvalue = requires(T const value) {
    std::move(value).take();
};

int main() {
    slot<int> value{1};
    const slot<int> const_value{2};
    static_assert(std::same_as<decltype(std::move(const_value).value()), const int&&>);
    static_assert(!can_take_lvalue<slot<std::unique_ptr<int>>>);
    static_assert(!can_take_const_lvalue<slot<std::unique_ptr<int>>>);
    static_assert(!can_take_const_rvalue<slot<std::unique_ptr<int>>>);
    static_assert(noexcept(value.value()));
    static_assert(noexcept(std::move(value).take()));

    name_holder holder{"borrowed"};
    decltype(auto) lvalue_name = holder.name();
    const name_holder const_holder{"const"};
    decltype(auto) const_name = const_holder.name();
    static_assert(std::same_as<decltype(lvalue_name), std::string&>);
    static_assert(std::same_as<decltype(const_name), const std::string&>);
    return &lvalue_name == holder.address() && &const_name == const_holder.address() ? 0 : 1;
}
