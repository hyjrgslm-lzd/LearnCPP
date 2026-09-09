#pragma once

#include "../../checks/support/rc_support.hpp"

#include <utility>

namespace l10 {

namespace detail {
struct bad_access;
}

template <class T>
struct bad_block {
    template <class... Args>
    explicit bad_block(Args&&... args) : object(new T(std::forward<Args>(args)...)) {
        l10_support::control_block_created();
    }

    ~bad_block() {
        delete object;
        l10_support::control_block_destroyed();
    }

    T* object = nullptr;
    int strong = 1;
    int weak = 1;
};

template <class T>
class weak_rc;

template <class T>
class rc_ptr {
public:
    rc_ptr() = default;
    rc_ptr(const rc_ptr& other) noexcept : block_(other.block_) { add_strong(); }
    rc_ptr& operator=(const rc_ptr& other) noexcept {
        auto* incoming = other.block_;
        if (incoming) {
            ++incoming->strong;
        }
        release();
        block_ = incoming;
        return *this;
    }
    rc_ptr(rc_ptr&& other) noexcept : block_(std::exchange(other.block_, nullptr)) {}
    rc_ptr& operator=(rc_ptr&& other) noexcept {
        if (this != &other) {
            auto* incoming = std::exchange(other.block_, nullptr);
            release();
            block_ = incoming;
        }
        return *this;
    }
    ~rc_ptr() { release(); }

    [[nodiscard]] T* get() const noexcept { return block_ ? block_->object : nullptr; }
    [[nodiscard]] T& operator*() const noexcept { return *get(); }
    [[nodiscard]] T* operator->() const noexcept { return get(); }
    [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }
    [[nodiscard]] int use_count() const noexcept { return block_ ? block_->strong : 0; }
    [[nodiscard]] int weak_count() const noexcept { return block_ ? block_->weak - 1 : 0; }
    void reset() noexcept { release(); }

private:
    friend class weak_rc<T>;
    friend struct detail::bad_access;

    explicit rc_ptr(bad_block<T>* block, bool add_ref) noexcept : block_(block) {
        if (add_ref) {
            add_strong();
        }
    }

    void add_strong() noexcept {
        if (block_) {
            ++block_->strong;
        }
    }
    void release() noexcept {
        auto* old = std::exchange(block_, nullptr);
        if (!old) {
            return;
        }
        --old->strong;
        if (old->strong == 0 && old->weak == 1) {
            --old->weak;
            delete old;
        }
    }
    bad_block<T>* block_ = nullptr;
};

namespace detail {
struct bad_access {
    template <class T>
    [[nodiscard]] static rc_ptr<T> adopt(bad_block<T>* block, bool add_ref) noexcept {
        return rc_ptr<T>(block, add_ref);
    }
};
} // namespace detail

template <class T>
class weak_rc {
public:
    weak_rc() = default;
    weak_rc(const rc_ptr<T>& owner) noexcept : block_(owner.block_) {
        if (block_) {
            ++block_->weak;
        }
    }
    weak_rc(const weak_rc& other) noexcept : block_(other.block_) {
        if (block_) {
            ++block_->weak;
        }
    }
    weak_rc& operator=(const weak_rc& other) noexcept {
        if (this != &other) {
            release();
            block_ = other.block_;
            if (block_) {
                ++block_->weak;
            }
        }
        return *this;
    }
    weak_rc(weak_rc&& other) noexcept : block_(std::exchange(other.block_, nullptr)) {}
    weak_rc& operator=(weak_rc&& other) noexcept {
        if (this != &other) {
            release();
            block_ = std::exchange(other.block_, nullptr);
        }
        return *this;
    }
    ~weak_rc() { release(); }

    [[nodiscard]] bool expired() const noexcept { return block_ == nullptr || block_->strong == 0; }
    [[nodiscard]] int use_count() const noexcept { return block_ ? block_->strong : 0; }
    [[nodiscard]] rc_ptr<T> lock() const noexcept { return expired() ? rc_ptr<T>{} : rc_ptr<T>(block_, true); }
    void reset() noexcept { release(); }

private:
    void release() noexcept {
        auto* old = std::exchange(block_, nullptr);
        if (!old) {
            return;
        }
        --old->weak;
        if (old->weak == 0 && old->strong == 0) {
            delete old;
        }
    }
    bad_block<T>* block_ = nullptr;
};

template <class T, class... Args>
[[nodiscard]] rc_ptr<T> make_rc(Args&&... args) {
    return detail::bad_access::adopt(new bad_block<T>(std::forward<Args>(args)...), false);
}

} // namespace l10
