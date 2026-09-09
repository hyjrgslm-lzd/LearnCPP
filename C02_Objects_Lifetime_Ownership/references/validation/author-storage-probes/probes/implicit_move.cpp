#include <iostream>
#include <type_traits>

struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

MoveOnly make_move_only()
{
    MoveOnly local;
    return local;
}

struct Base {
    Base() = default;
    Base(const Base&) = delete;
    Base(Base&&) noexcept = default;
};

struct Derived : Base {};

Base return_base_from_derived()
{
    Derived local;
    return local;
}

int main()
{
    static_assert(std::is_move_constructible_v<MoveOnly>);
    static_assert(!std::is_copy_constructible_v<MoveOnly>);
    [[maybe_unused]] auto a = make_move_only();
    [[maybe_unused]] auto b = return_base_from_derived();
    std::cout << "implicit_move OK\n";
}
