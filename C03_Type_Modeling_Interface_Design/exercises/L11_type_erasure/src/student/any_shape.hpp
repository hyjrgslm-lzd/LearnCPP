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
        using Stored = std::remove_cvref_t<T>;
        object_ = new Stored(std::forward<T>(value));
        ops_ = &table_for<Stored>;
    }

    AnyShape(const AnyShape& other)
        : object_(other.object_ == nullptr ? nullptr : other.ops_->clone(other.object_)), ops_(other.ops_)
    {
    }

    AnyShape(AnyShape&& other) noexcept : object_(std::exchange(other.object_, nullptr)), ops_(std::exchange(other.ops_, nullptr))
    {
    }

    AnyShape& operator=(const AnyShape& other)
    {
        if (this != &other) {
            reset();
            object_ = other.object_ == nullptr ? nullptr : other.ops_->clone(other.object_);
            ops_ = other.ops_;
        }
        return *this;
    }

    AnyShape& operator=(AnyShape&& other) noexcept
    {
        if (this != &other) {
            reset();
            object_ = std::exchange(other.object_, nullptr);
            ops_ = std::exchange(other.ops_, nullptr);
        }
        return *this;
    }

    ~AnyShape() { reset(); }

    explicit operator bool() const noexcept { return object_ != nullptr; }

    std::string name() const
    {
        ensure_value();
        return ops_->name(object_);
    }

    Dimensions dimensions() const
    {
        ensure_value();
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
    inline static const Ops table_for{
        [](const void* object) -> void* { return new T(*static_cast<const T*>(object)); },
        [](void* object) noexcept { delete static_cast<T*>(object); },
        [](const void* object) -> std::string { return static_cast<const T*>(object)->name(); },
        [](const void* object) -> Dimensions { return static_cast<const T*>(object)->dimensions(); },
    };

    void ensure_value() const
    {
        if (object_ == nullptr) {
            throw std::logic_error("empty AnyShape");
        }
    }

    void reset() noexcept
    {
        if (object_ != nullptr) {
            ops_->destroy(object_);
            object_ = nullptr;
            ops_ = nullptr;
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

