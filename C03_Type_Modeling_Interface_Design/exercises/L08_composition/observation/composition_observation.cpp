#include <check.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct Dimensions {
    int width;
    int height;

    friend bool operator==(Dimensions, Dimensions) = default;
};

class Rectangle {
public:
    Rectangle(int width, int height) : dimensions_{width, height}
    {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("positive dimensions required");
        }
    }

    Dimensions dimensions() const noexcept { return dimensions_; }

private:
    Dimensions dimensions_;
};

class LabelledRectangle {
public:
    LabelledRectangle(std::string label, Rectangle rectangle)
        : label_(std::move(label)), rectangle_(rectangle)
    {
    }

    const std::string& label() const noexcept { return label_; }
    Dimensions dimensions() const noexcept { return rectangle_.dimensions(); }

private:
    std::string label_;
    Rectangle rectangle_;
};

class ShapeView {
public:
    Dimensions dimensions() const
    {
        Dimensions result = do_dimensions();
        if (result.width <= 0 || result.height <= 0) {
            throw std::logic_error("derived shape broke dimensions contract");
        }
        return result;
    }

    virtual ~ShapeView() = default;

private:
    virtual Dimensions do_dimensions() const = 0;
};

class GoodShape final : public ShapeView {
private:
    Dimensions do_dimensions() const override { return {3, 4}; }
};

class BadShape final : public ShapeView {
private:
    Dimensions do_dimensions() const override { return {0, 4}; }
};

class ConstructLogBase {
public:
    ConstructLogBase() : observed_(name()) {}
    virtual ~ConstructLogBase() = default;

    const std::string& observed() const noexcept { return observed_; }
    virtual std::string name() const { return "base"; }

private:
    std::string observed_;
};

class ConstructLogDerived final : public ConstructLogBase {
public:
    std::string name() const override { return "derived"; }
};

} // namespace

int main()
{
    LabelledRectangle labelled{"hero", Rectangle{3, 4}};
    check(labelled.label() == "hero", "composition keeps label as element state");
    check(labelled.dimensions() == Dimensions{3, 4}, "composition forwards only chosen rectangle observation");

    const ShapeView& shape = GoodShape{};
    check(shape.dimensions() == Dimensions{3, 4}, "public inheritance supports base substitution");

    bool rejected = false;
    try {
        static_cast<void>(BadShape{}.dimensions());
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, "NVI checks derived postcondition at public boundary");

    ConstructLogDerived derived;
    check(derived.name() == "derived", "virtual dispatch works after construction");
    check(derived.observed() == "base", "base constructor does not dispatch to derived override");
}
