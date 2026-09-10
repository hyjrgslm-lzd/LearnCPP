#pragma once

#include <span>
#include <stdexcept>
#include <type_traits>

namespace c06_l05 {

template <class T>
class dynamic_array {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
    static_assert(std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>,
        "T must be copy constructible or nothrow move constructible");

public:
    dynamic_array() = default;
    dynamic_array(const dynamic_array&) = delete;
    dynamic_array& operator=(const dynamic_array&) = delete;
    dynamic_array(dynamic_array&&) noexcept = default;
    dynamic_array& operator=(dynamic_array&&) noexcept = default;

    [[nodiscard]] std::size_t size() const noexcept { return 0; }
    [[nodiscard]] std::size_t capacity() const noexcept { return 0; }
    [[nodiscard]] bool empty() const noexcept { return true; }

    T& operator[](std::size_t) { throw std::logic_error("student dynamic_array not implemented"); }
    const T& operator[](std::size_t) const { throw std::logic_error("student dynamic_array not implemented"); }

    [[nodiscard]] std::span<T> view() noexcept { return {}; }
    [[nodiscard]] std::span<const T> view() const noexcept { return {}; }

    void reserve(std::size_t) {}
    void push_back(T) { throw std::logic_error("student dynamic_array not implemented"); }
    void pop_back() { throw std::out_of_range("student dynamic_array not implemented"); }
    void clear() noexcept {}
};

} // namespace c06_l05
