#pragma once

#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace l12 {

template <class T>
class storage_slot {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

public:
    storage_slot() = default;
    storage_slot(const storage_slot&) = delete;
    storage_slot& operator=(const storage_slot&) = delete;

    ~storage_slot() noexcept
    {
        destroy();
    }

    template <class... Args>
    T& construct(Args&&... args)
    {
        if (engaged_) {
            throw std::logic_error("storage_slot already occupied");
        }
        T* object = std::construct_at(ptr(), std::forward<Args>(args)...);
        engaged_ = true;
        return *object;
    }

    void destroy() noexcept
    {
        if (!engaged_) {
            return;
        }
        std::destroy_at(ptr());
        engaged_ = false;
    }

    [[nodiscard]] bool engaged() const noexcept
    {
        return engaged_;
    }

    T& get()
    {
        if (!engaged_) {
            throw std::logic_error("storage_slot is empty");
        }
        return *ptr();
    }

    const T& get() const
    {
        if (!engaged_) {
            throw std::logic_error("storage_slot is empty");
        }
        return *ptr();
    }

private:
    T* ptr() noexcept
    {
        return reinterpret_cast<T*>(&storage_);
    }

    const T* ptr() const noexcept
    {
        return reinterpret_cast<const T*>(&storage_);
    }

    alignas(T) unsigned char storage_[sizeof(T)]{};
    bool engaged_ = false;
};

} // namespace l12
