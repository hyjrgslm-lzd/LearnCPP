#pragma once

#include "../../checks/support/rc_support.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace l10 {

template <class T>
class rc_ptr;

template <class T>
class weak_rc;

namespace detail {
struct rc_access;
}

class control_block_base {
public:
    control_block_base() noexcept { l10_support::control_block_created(); }
    control_block_base(const control_block_base&) = delete;
    control_block_base& operator=(const control_block_base&) = delete;
    virtual ~control_block_base() { l10_support::control_block_destroyed(); }

    int strong = 1;
    int weak = 1;
    bool object_alive = true;

    virtual void destroy_object() noexcept = 0;
    virtual void* object_ptr() noexcept = 0;
};

template <class T>
class control_block final : public control_block_base {
public:
    template <class... Args>
    explicit control_block(Args&&... args) {
        std::construct_at(ptr(), std::forward<Args>(args)...);
    }

    ~control_block() override = default;

    void destroy_object() noexcept override {
        if (object_alive) {
            std::destroy_at(ptr());
            object_alive = false;
        }
    }

    void* object_ptr() noexcept override {
        return ptr();
    }

private:
    T* ptr() noexcept {
        return reinterpret_cast<T*>(storage_);
    }

    alignas(T) std::byte storage_[sizeof(T)];
};

template <class T>
class rc_ptr {
public:
    rc_ptr() = default;
    rc_ptr(std::nullptr_t) noexcept {}

    rc_ptr(const rc_ptr& other) noexcept : block_(other.block_) {
        add_strong();
    }

    rc_ptr& operator=(const rc_ptr& other) noexcept {
        auto* incoming = other.block_;
        if (incoming != nullptr) {
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

    [[nodiscard]] T* get() const noexcept {
        return block_ && block_->object_alive ? static_cast<T*>(block_->object_ptr()) : nullptr;
    }

    [[nodiscard]] T& operator*() const noexcept { return *get(); }
    [[nodiscard]] T* operator->() const noexcept { return get(); }
    [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }
    [[nodiscard]] int use_count() const noexcept { return block_ ? block_->strong : 0; }
    [[nodiscard]] int weak_count() const noexcept { return block_ ? block_->weak - 1 : 0; }

    void reset() noexcept {
        release();
    }

private:
    friend class weak_rc<T>;
    friend struct detail::rc_access;

    explicit rc_ptr(control_block_base* block, bool add_ref) noexcept : block_(block) {
        if (add_ref) {
            add_strong();
        }
    }

    void add_strong() noexcept {
        if (block_ != nullptr) {
            ++block_->strong;
        }
    }

    void release() noexcept {
        auto* old = std::exchange(block_, nullptr);
        if (old == nullptr) {
            return;
        }
        --old->strong;
        if (old->strong == 0) {
            old->destroy_object();
            --old->weak;
            if (old->weak == 0) {
                delete old;
            }
        }
    }

    control_block_base* block_ = nullptr;
};

namespace detail {
struct rc_access {
    template <class T>
    [[nodiscard]] static rc_ptr<T> adopt(control_block_base* block, bool add_ref) noexcept {
        return rc_ptr<T>(block, add_ref);
    }
};
} // namespace detail

template <class T>
class weak_rc {
public:
    weak_rc() = default;

    weak_rc(const rc_ptr<T>& owner) noexcept : block_(owner.block_) {
        add_weak();
    }

    weak_rc(const weak_rc& other) noexcept : block_(other.block_) {
        add_weak();
    }

    weak_rc& operator=(const weak_rc& other) noexcept {
        if (this != &other) {
            release();
            block_ = other.block_;
            add_weak();
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

    [[nodiscard]] bool expired() const noexcept {
        return block_ == nullptr || block_->strong == 0;
    }

    [[nodiscard]] int use_count() const noexcept {
        return block_ ? block_->strong : 0;
    }

    [[nodiscard]] rc_ptr<T> lock() const noexcept {
        if (expired()) {
            return {};
        }
        return rc_ptr<T>(block_, true);
    }

    void reset() noexcept {
        release();
    }

private:
    void add_weak() noexcept {
        if (block_ != nullptr) {
            ++block_->weak;
        }
    }

    void release() noexcept {
        auto* old = std::exchange(block_, nullptr);
        if (old == nullptr) {
            return;
        }
        --old->weak;
        if (old->weak == 0 && old->strong == 0) {
            delete old;
        }
    }

    control_block_base* block_ = nullptr;
};

template <class T, class... Args>
[[nodiscard]] rc_ptr<T> make_rc(Args&&... args) {
    return detail::rc_access::adopt<T>(new control_block<T>(std::forward<Args>(args)...), false);
}

} // namespace l10
