#pragma once

namespace c04_overload {

struct member_result {
    int calls{};
};

template<unsigned long long N>
struct literal_result {
    static constexpr unsigned long long extent = N;
};

struct integral_result {};
struct range_result {};

struct describe_fn {
    template<class T>
    constexpr integral_result operator()(T) const noexcept {
        return {};
    }
};

inline constexpr describe_fn describe{};

} // namespace c04_overload
