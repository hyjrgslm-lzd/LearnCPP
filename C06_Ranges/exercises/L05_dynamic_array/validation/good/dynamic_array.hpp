#pragma once

#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

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

    [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return values_.capacity(); }
    [[nodiscard]] bool empty() const noexcept { return values_.empty(); }

    T& operator[](std::size_t index) noexcept { return values_[index]; }
    const T& operator[](std::size_t index) const noexcept { return values_[index]; }

    [[nodiscard]] std::span<T> view() noexcept { return {values_.data(), values_.size()}; }
    [[nodiscard]] std::span<const T> view() const noexcept { return {values_.data(), values_.size()}; }

    void reserve(std::size_t requested) {
        values_.reserve(requested);
    }

    void push_back(T value) {
        values_.push_back(std::move(value));
    }

    void pop_back() {
        if (values_.empty()) {
            throw std::out_of_range("dynamic_array::pop_back on empty array");
        }
        values_.pop_back();
    }

    void clear() noexcept {
        values_.clear();
    }

private:
    std::vector<T> values_;
};

} // namespace c06_l05
