#pragma once

#include "shape_types.hpp"

#include <concepts>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace l11 {

template<class T>
concept ShapeObject = std::copy_constructible<T> && std::is_nothrow_destructible_v<T> &&
    requires(const T& value) {
        { value.name() } -> std::convertible_to<std::string>;
        { value.dimensions() } -> std::same_as<Dimensions>;
    };

class AnyShape {
public:
    AnyShape() noexcept = default;

    template<class T>
        requires(!std::same_as<std::remove_cvref_t<T>, AnyShape> && ShapeObject<std::remove_cvref_t<T>>)
    AnyShape(T&& value)
    {
        emplace<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    AnyShape(const AnyShape& other)
    {
        if (other.object_ != nullptr) {
            object_ = other.ops_->clone(other.object_);
            ops_ = other.ops_;
        }
    }

    AnyShape(AnyShape&& other) noexcept { swap(other); }

    AnyShape& operator=(const AnyShape& other)
    {
        if (this != &other) {
            AnyShape next(other);
            swap(next);
        }
        return *this;
    }

    AnyShape& operator=(AnyShape&& other) noexcept
    {
        if (this != &other) {
            AnyShape next(std::move(other));
            swap(next);
        }
        return *this;
    }

    ~AnyShape() { clear(); }

    explicit operator bool() const noexcept { return object_ != nullptr; }

    std::string name() const
    {
        require_value();
        return ops_->name(object_);
    }

    Dimensions dimensions() const
    {
        require_value();
        return ops_->dimensions(object_);
    }

    const void* target_address() const noexcept { return object_; }

    void swap(AnyShape& other) noexcept
    {
        std::swap(object_, other.object_);
        std::swap(ops_, other.ops_);
    }

private:
    struct Ops {
        void* (*clone)(const void*);
        void (*destroy)(void*) noexcept;
        std::string (*name)(const void*);
        Dimensions (*dimensions)(const void*);
    };

    template<ShapeObject T>
    inline static const Ops ops_for{
        [](const void* object) -> void* { return new T(*static_cast<const T*>(object)); },
        [](void* object) noexcept { delete static_cast<T*>(object); },
        [](const void* object) { return static_cast<const T*>(object)->name(); },
        [](const void* object) { return static_cast<const T*>(object)->dimensions(); },
    };

    template<ShapeObject T>
    void emplace(T&& value)
    {
        object_ = new std::remove_cvref_t<T>(std::forward<T>(value));
        ops_ = &ops_for<std::remove_cvref_t<T>>;
    }

    void require_value() const
    {
        if (object_ == nullptr) {
            throw std::logic_error("empty AnyShape");
        }
    }

    void clear() noexcept
    {
        if (object_ != nullptr) {
            ops_->destroy(object_);
        }
    }

    void* object_ = nullptr;
    const Ops* ops_ = nullptr;
};

inline void swap(AnyShape& left, AnyShape& right) noexcept
{
    left.swap(right);
}

} // namespace l11

