#pragma once

#include "../../checks/support/rc_support.hpp"

namespace l10 {

template <class T>
class rc_ptr {
public:
    rc_ptr() = default;
    [[nodiscard]] T* get() const noexcept { return nullptr; }
    [[nodiscard]] T& operator*() const noexcept { return *get(); }
    [[nodiscard]] T* operator->() const noexcept { return get(); }
    [[nodiscard]] explicit operator bool() const noexcept { return false; }
    [[nodiscard]] int use_count() const noexcept { return 0; }
    [[nodiscard]] int weak_count() const noexcept { return 0; }
    void reset() noexcept {}
};

template <class T>
class weak_rc {
public:
    weak_rc() = default;
    weak_rc(const rc_ptr<T>&) noexcept {}
    [[nodiscard]] bool expired() const noexcept { return true; }
    [[nodiscard]] int use_count() const noexcept { return 0; }
    [[nodiscard]] rc_ptr<T> lock() const noexcept { return {}; }
    void reset() noexcept {}
};

template <class T, class... Args>
[[nodiscard]] rc_ptr<T> make_rc(Args&&...) {
    return {};
}

} // namespace l10

