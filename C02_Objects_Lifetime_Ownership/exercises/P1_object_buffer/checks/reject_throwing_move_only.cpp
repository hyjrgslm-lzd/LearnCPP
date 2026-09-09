#include <object_buffer.hpp>

struct ThrowingMoveOnly {
    ThrowingMoveOnly() = default;
    ThrowingMoveOnly(const ThrowingMoveOnly&) = delete;
    ThrowingMoveOnly(ThrowingMoveOnly&&) noexcept(false) {}
    ~ThrowingMoveOnly() noexcept = default;
};

p1::object_buffer<ThrowingMoveOnly> buffer;

int main()
{
    return static_cast<int>(buffer.size());
}
