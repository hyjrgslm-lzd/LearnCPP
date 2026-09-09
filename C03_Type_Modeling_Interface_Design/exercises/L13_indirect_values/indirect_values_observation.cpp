#include <check.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {

struct Shape {
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual int area() const = 0;
    virtual void scale(int factor) = 0;
};

struct Square final : Shape {
    int side;
    bool throw_on_clone = false;

    explicit Square(int side_value, bool should_throw = false) : side(side_value), throw_on_clone(should_throw) {}

    std::unique_ptr<Shape> clone() const override
    {
        if (throw_on_clone) {
            throw std::runtime_error("planned clone failure");
        }
        return std::make_unique<Square>(*this);
    }

    int area() const override { return side * side; }
    void scale(int factor) override { side *= factor; }
};

class clone_value {
public:
    clone_value() = default;
    explicit clone_value(std::unique_ptr<Shape> shape) : shape_(std::move(shape)) {}

    clone_value(const clone_value& other)
        : shape_(other.shape_ ? other.shape_->clone() : nullptr)
    {
    }

    clone_value& operator=(const clone_value& other)
    {
        if (this == &other) {
            return *this;
        }
        std::unique_ptr<Shape> next = other.shape_ ? other.shape_->clone() : nullptr;
        shape_ = std::move(next);
        return *this;
    }

    clone_value(clone_value&&) noexcept = default;
    clone_value& operator=(clone_value&&) noexcept = default;

    explicit operator bool() const noexcept { return static_cast<bool>(shape_); }

    Shape& get()
    {
        if (!shape_) {
            throw std::logic_error("empty clone_value");
        }
        return *shape_;
    }

    const Shape& get() const
    {
        if (!shape_) {
            throw std::logic_error("empty clone_value");
        }
        return *shape_;
    }

private:
    std::unique_ptr<Shape> shape_;
};

void check_ownership_models()
{
    auto unique = std::make_unique<Square>(3);
    auto moved = std::move(unique);
    check(unique == nullptr, "unique_ptr move transfers ownership");
    check(moved->area() == 9, "moved unique_ptr owns the object");

    auto shared = std::make_shared<Square>(4);
    std::shared_ptr<Square> alias = shared;
    alias->scale(2);
    check(shared->area() == 64, "shared_ptr copy shares one object");

    clone_value value{std::make_unique<Square>(5)};
    clone_value copy = value;
    copy.get().scale(2);
    check(value.get().area() == 25, "clone_value copy keeps original independent");
    check(copy.get().area() == 100, "clone_value copy owns cloned dynamic object");
}

void check_const_propagation()
{
    const clone_value value{std::make_unique<Square>(6)};
    static_assert(std::is_const_v<std::remove_reference_t<decltype(value.get())>>);
    check(value.get().area() == 36, "const clone_value reads through const reference");
}

void check_clone_failure_keeps_target()
{
    clone_value stable{std::make_unique<Square>(7)};
    clone_value throwing{std::make_unique<Square>(9, true)};

    bool threw = false;
    try {
        stable = throwing;
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "clone failure is reported");
    check(stable.get().area() == 49, "clone failure keeps original value");
}

void check_moved_from_empty_state()
{
    clone_value value{std::make_unique<Square>(8)};
    clone_value moved = std::move(value);
    check(moved.get().area() == 64, "moved clone_value keeps dynamic object");
    check(!value, "moved-from teaching clone_value is valueless");
}

} // namespace

int main()
{
    check_ownership_models();
    check_const_propagation();
    check_clone_failure_keeps_target();
    check_moved_from_empty_state();
}
