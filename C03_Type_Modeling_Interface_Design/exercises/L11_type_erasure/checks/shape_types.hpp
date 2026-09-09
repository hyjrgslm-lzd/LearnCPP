#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace l11 {

struct Dimensions {
    int width;
    int height;

    friend bool operator==(Dimensions, Dimensions) = default;
};

class Rectangle {
public:
    Rectangle(std::string label, int width, int height)
        : label_(std::move(label)), dimensions_{width, height}
    {
        if (label_.empty() || width <= 0 || height <= 0) {
            throw std::invalid_argument("valid rectangle required");
        }
    }

    std::string name() const { return label_; }
    Dimensions dimensions() const { return dimensions_; }

private:
    std::string label_;
    Dimensions dimensions_;
};

class CountingShape {
public:
    CountingShape(std::string label, Dimensions dimensions)
        : label_(std::move(label)), dimensions_(dimensions)
    {
        if (label_.empty() || dimensions.width <= 0 || dimensions.height <= 0) {
            throw std::invalid_argument("valid counting shape required");
        }
        ++live_;
    }

    CountingShape(const CountingShape& other) : label_(other.label_), dimensions_(other.dimensions_)
    {
        ++copies_;
        if (copies_ == throw_at_) {
            throw std::runtime_error("planned copy failure");
        }
        ++live_;
    }

    CountingShape(CountingShape&& other) noexcept
        : label_(std::move(other.label_)), dimensions_(other.dimensions_)
    {
        ++live_;
    }

    CountingShape& operator=(const CountingShape&) = delete;
    CountingShape& operator=(CountingShape&&) = delete;

    ~CountingShape() noexcept
    {
        --live_;
        ++destroyed_;
    }

    std::string name() const { return label_; }
    Dimensions dimensions() const { return dimensions_; }

    static void reset() noexcept
    {
        live_ = 0;
        destroyed_ = 0;
        copies_ = 0;
        throw_at_ = -1;
    }

    static void reset_copy_plan() noexcept
    {
        copies_ = 0;
        throw_at_ = -1;
    }

    static void throw_on_copy(int copy_index) noexcept { throw_at_ = copy_index; }
    static int live_count() noexcept { return live_; }
    static int destroyed_count() noexcept { return destroyed_; }

private:
    std::string label_;
    Dimensions dimensions_;
    inline static int live_ = 0;
    inline static int destroyed_ = 0;
    inline static int copies_ = 0;
    inline static int throw_at_ = -1;
};

class alignas(64) OverAlignedShape {
public:
    OverAlignedShape(std::string label, Dimensions dimensions)
        : label_(std::move(label)), dimensions_(dimensions)
    {
        if (label_.empty() || dimensions.width <= 0 || dimensions.height <= 0) {
            throw std::invalid_argument("valid over-aligned shape required");
        }
        ++live_;
    }

    OverAlignedShape(const OverAlignedShape& other) : label_(other.label_), dimensions_(other.dimensions_)
    {
        ++live_;
    }

    OverAlignedShape(OverAlignedShape&& other) noexcept
        : label_(std::move(other.label_)), dimensions_(other.dimensions_)
    {
        ++live_;
    }

    OverAlignedShape& operator=(const OverAlignedShape&) = delete;
    OverAlignedShape& operator=(OverAlignedShape&&) = delete;

    ~OverAlignedShape() noexcept { --live_; }

    std::string name() const { return label_; }
    Dimensions dimensions() const { return dimensions_; }

    static void reset() noexcept { live_ = 0; }
    static int live_count() noexcept { return live_; }

private:
    std::string label_;
    Dimensions dimensions_;
    inline static int live_ = 0;
};

} // namespace l11
