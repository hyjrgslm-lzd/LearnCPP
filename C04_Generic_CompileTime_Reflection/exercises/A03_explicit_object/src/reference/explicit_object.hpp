#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace c04_explicit {
template<class Like, class T>
constexpr decltype(auto) forward_like(T&& value) noexcept {
    using U = std::remove_reference_t<T>;
    using L = std::remove_reference_t<Like>;
    if constexpr (std::is_lvalue_reference_v<Like>) {
        if constexpr (std::is_const_v<L>) return static_cast<const U&>(value);
        else return static_cast<U&>(value);
    } else {
        if constexpr (std::is_const_v<L>) return static_cast<const U&&>(value);
        else return static_cast<U&&>(value);
    }
}

template<class T>
class slot {
public:
    explicit slot(T value) : value_(std::move(value)) {}

    constexpr decltype(auto) value(this auto&& self) noexcept {
        return ::c04_explicit::forward_like<decltype(self)>(self.value_);
    }

    T take(this slot&& self) noexcept {
        return std::move(self.value_);
    }
    T take(this slot&) = delete;
    T take(this const slot&) = delete;
    T take(this const slot&&) = delete;

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

    int sum(this const chain& self) noexcept {
        auto walk = [](this auto const& again, const chain* node) noexcept -> int {
            return node == nullptr ? 0 : node->value + again(node->next.get());
        };
        return walk(&self);
    }
};

class name_holder {
public:
    explicit name_holder(std::string name) : name_(std::move(name)) {}

    decltype(auto) name(this auto&& self) noexcept {
        return ::c04_explicit::forward_like<decltype(self)>(self.name_);
    }

    std::string* address() noexcept { return &name_; }
    const std::string* address() const noexcept { return &name_; }

private:
    std::string name_;
};
}
