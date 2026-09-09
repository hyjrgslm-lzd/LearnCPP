#pragma once

#include "../../checks/support/rc_support.hpp"

#include <utility>

namespace l10 {

namespace detail {
struct bad_access;
}

template <class T>
class weak_rc;

template <class T>
class rc_ptr {
public:
    rc_ptr() = default;
    rc_ptr(const rc_ptr& other) noexcept : ptr_(other.ptr_), strong_(other.strong_) {
        if (strong_) {
            ++*strong_;
        }
    }
    rc_ptr& operator=(const rc_ptr& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            strong_ = other.strong_;
            if (strong_) {
                ++*strong_;
            }
        }
        return *this;
    }
    rc_ptr(rc_ptr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)),
          strong_(std::exchange(other.strong_, nullptr)) {}
    rc_ptr& operator=(rc_ptr&& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = std::exchange(other.ptr_, nullptr);
            strong_ = std::exchange(other.strong_, nullptr);
        }
        return *this;
    }
    ~rc_ptr() { release(); }

    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
    [[nodiscard]] T* operator->() const noexcept { return ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }
    [[nodiscard]] int use_count() const noexcept { return strong_ ? *strong_ : 0; }
    [[nodiscard]] int weak_count() const noexcept { return 0; }
    void reset() noexcept { release(); }

private:
    friend class weak_rc<T>;
    friend struct detail::bad_access;

    rc_ptr(T* ptr, int* strong) noexcept : ptr_(ptr), strong_(strong) {}

    void release() noexcept {
        if (strong_ && --*strong_ == 0) {
            delete ptr_;
            delete strong_;
            ptr_ = nullptr;
            strong_ = nullptr;
        }
    }
    T* ptr_ = nullptr;
    int* strong_ = nullptr;
};

namespace detail {
struct bad_access {
    template <class T>
    [[nodiscard]] static rc_ptr<T> adopt(T* ptr, int* strong) noexcept {
        return rc_ptr<T>(ptr, strong);
    }
};
} // namespace detail

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
[[nodiscard]] rc_ptr<T> make_rc(Args&&... args) {
    l10_support::control_block_created();
    return detail::bad_access::adopt(new T(std::forward<Args>(args)...), new int(1));
}

} // namespace l10
