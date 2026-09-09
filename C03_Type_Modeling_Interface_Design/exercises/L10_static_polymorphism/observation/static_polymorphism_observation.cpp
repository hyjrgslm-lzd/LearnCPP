#include <check.hpp>

#include <concepts>
#include <string>

namespace {

struct Dimensions {
    int width;
    int height;
};

template<class T>
concept ShapeLike = requires(const T& shape) {
    { shape.name() } -> std::convertible_to<std::string>;
    { shape.dimensions() } -> std::same_as<Dimensions>;
};

template<ShapeLike T>
std::string describe(const T& shape)
{
    Dimensions d = shape.dimensions();
    return std::string(shape.name()) + ":" + std::to_string(d.width) + "x" + std::to_string(d.height);
}

class Rectangle {
public:
    std::string name() const { return "rectangle"; }
    Dimensions dimensions() const { return {3, 4}; }
};

class MissingName {
public:
    Dimensions dimensions() const { return {1, 1}; }
};

template<class Derived>
class ShapeFacade {
public:
    std::string describe() const
    {
        const auto& self = static_cast<const Derived&>(*this);
        Dimensions d = self.dimensions_impl();
        return self.name_impl() + ":" + std::to_string(d.width) + "x" + std::to_string(d.height);
    }
};

class CrtpCircle final : public ShapeFacade<CrtpCircle> {
public:
    std::string name_impl() const { return "circle"; }
    Dimensions dimensions_impl() const { return {6, 6}; }
};

template<class T>
class DependentBase {
public:
    void mark() { marked_ = true; }
    bool marked() const noexcept { return marked_; }

private:
    bool marked_ = false;
};

template<class T>
class DependentDerived final : public DependentBase<T> {
public:
    void run()
    {
        this->mark();
    }
};

} // namespace

int main()
{
    static_assert(ShapeLike<Rectangle>);
    static_assert(!ShapeLike<MissingName>);

    check(describe(Rectangle{}) == "rectangle:3x4", "template function instantiates for ShapeLike type");
    check(CrtpCircle{}.describe() == "circle:6x6", "CRTP facade dispatches to derived implementation");

    DependentDerived<int> value;
    value.run();
    check(value.marked(), "dependent base member is found through this");
}

