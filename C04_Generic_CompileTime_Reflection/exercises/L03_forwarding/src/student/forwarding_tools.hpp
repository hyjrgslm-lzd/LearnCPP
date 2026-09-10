#pragma once

#include <initializer_list>
#include <utility>
#include <vector>

namespace l03 {

template<class F, class... Args>
constexpr int invoke_preserving(F&&, Args&&...) noexcept {
    return 0;
}

template<class T>
struct holder {
    T value;
};

template<class T>
holder(T) -> holder<T>;

template<class T>
constexpr holder<std::vector<T>> make_list_holder(std::initializer_list<T>) {
    return holder<std::vector<T>>{{}};
}

} // namespace l03
