#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace c04_explicit {
template<class Like, class T>
constexpr decltype(auto) forward_like(T&& value) noexcept {
    using U = std::remove_reference_t<T>;
    if constexpr (std::is_const_v<std::remove_reference_t<Like>>) {
        if constexpr (std::is_lvalue_reference_v<Like>) return static_cast<const U&>(value);
        else return static_cast<const U&&>(value);
    } else {
        if constexpr (std::is_lvalue_reference_v<Like>) return static_cast<U&>(value);
        else return static_cast<U&&>(value);
    }
}

template<class T>
class slot {
public:
    explicit slot(T value) : value_(std::move(value)) {}

    T value() & noexcept { return value_; }
    T value() const& noexcept { return value_; }
    T value() && noexcept { return std::move(value_); }
    T value() const&& noexcept { return value_; }

    T take() && noexcept { return std::move(value_); }
    T take() const&& = delete;

    [[nodiscard]] bool empty() const noexcept {
        if constexpr (requires { static_cast<bool>(value_); }) return !value_;
        else return false;
    }

    T* address() noexcept { return &value_; }
    const T* address() const noexcept { return &value_; }

private:
    T value_;
};

struct chain {
    int value{};
    std::unique_ptr<chain> next;

    explicit chain(int value_) : value(value_) {}
    int sum() const noexcept { return value; }
};

class name_holder {
public:
    explicit name_holder(std::string name) : name_(std::move(name)) {}

    std::string name() & noexcept { return name_; }
    const std::string name() const& noexcept { return name_; }
    std::string name() && noexcept { return std::move(name_); }
    const std::string name() const&& noexcept { return name_; }

    std::string* address() noexcept { return &name_; }
    const std::string* address() const noexcept { return &name_; }

private:
    std::string name_;
};
}
