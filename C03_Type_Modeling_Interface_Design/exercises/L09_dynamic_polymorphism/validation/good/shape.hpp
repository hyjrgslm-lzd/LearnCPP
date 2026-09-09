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
    Rectangle(int width, int height) : width_(width), height_(height)
    {
        if (width_ <= 0 || height_ <= 0) {
            throw std::invalid_argument("positive dimensions required");
        }
        ++alive_;
    }

    Rectangle(const Rectangle& other) : width_(other.width_), height_(other.height_) { ++alive_; }
    Rectangle& operator=(const Rectangle&) = default;
    ~Rectangle() override { --alive_; }

    std::string name() const override { return "rectangle"; }
    Dimensions dimensions() const override { return {width_, height_}; }
    std::unique_ptr<Shape> clone() const override { return std::unique_ptr<Shape>(new Rectangle(*this)); }

    static int alive_count() noexcept { return alive_; }

private:
    int width_;
    int height_;
    inline static int alive_ = 0;
};

class Circle final : public Shape {
public:
    explicit Circle(int radius) : diameter_(radius * 2)
    {
        if (radius <= 0 || radius > std::numeric_limits<int>::max() / 2) {
            throw std::invalid_argument("positive radius required");
        }
    }

    std::string name() const override { return "circle"; }
    Dimensions dimensions() const override { return {diameter_, diameter_}; }
    std::unique_ptr<Shape> clone() const override { return std::unique_ptr<Shape>(new Circle(*this)); }

private:
    int diameter_;
};

} // namespace l09
