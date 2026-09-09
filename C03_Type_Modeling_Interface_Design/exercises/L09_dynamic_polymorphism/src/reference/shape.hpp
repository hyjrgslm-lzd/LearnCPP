#pragma once

#include <memory>
#include <limits>
#include <stdexcept>
#include <string>

namespace l09 {

struct Dimensions {
    int width;
    int height;

    friend bool operator==(Dimensions, Dimensions) = default;
};

class Shape {
public:
    virtual std::string name() const = 0;
    virtual Dimensions dimensions() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual ~Shape() = default;
};

class Rectangle final : public Shape {
public:
    Rectangle(int width, int height) : dimensions_{width, height}
    {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("positive dimensions required");
        }
        ++alive_;
    }

    Rectangle(const Rectangle& other) : dimensions_(other.dimensions_) { ++alive_; }
    Rectangle& operator=(const Rectangle&) = default;
    ~Rectangle() override { --alive_; }

    std::string name() const override { return "rectangle"; }
    Dimensions dimensions() const override { return dimensions_; }

    std::unique_ptr<Shape> clone() const override
    {
        return std::make_unique<Rectangle>(*this);
    }

    static int alive_count() noexcept { return alive_; }

private:
    Dimensions dimensions_;
    inline static int alive_ = 0;
};

class Circle final : public Shape {
public:
    explicit Circle(int radius) : radius_(radius)
    {
        if (radius <= 0 || radius > std::numeric_limits<int>::max() / 2) {
            throw std::invalid_argument("positive radius required");
        }
    }

    std::string name() const override { return "circle"; }
    Dimensions dimensions() const override { return {radius_ * 2, radius_ * 2}; }

    std::unique_ptr<Shape> clone() const override
    {
        return std::make_unique<Circle>(*this);
    }

private:
    int radius_;
};

} // namespace l09
