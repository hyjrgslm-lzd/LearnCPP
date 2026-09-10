#pragma once

#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

namespace l03 {

template<class F, class... Args>
constexpr decltype(auto) invoke_preserving(F&& f, Args&&... args)
    noexcept(noexcept(std::invoke(std::forward<F>(f), std::move(args)...)))
{
    return std::invoke(std::forward<F>(f), std::move(args)...);
}

template<class T>
struct holder {
    T value;
};

template<class T>
holder(T) -> holder<T>;

template<class T>
constexpr holder<std::vector<T>> make_list_holder(std::initializer_list<T> values) {
    return holder<std::vector<T>>{std::vector<T>(values)};
}

} // namespace l03
