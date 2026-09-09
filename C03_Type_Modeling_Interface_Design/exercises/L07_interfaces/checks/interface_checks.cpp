#include <owner.hpp>

#include <check.hpp>

#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

template <class T>
constexpr bool rvalue_view_available = requires {
    std::declval<T&&>().view();
};

std::vector<int> copy_view(std::span<const int> values)
{
    return {values.begin(), values.end()};
}

} // namespace

int main()
{
    static_assert(noexcept(std::declval<const l07::Owner&>().view()));
    static_assert(noexcept(std::declval<l07::Owner&>().replace_all(std::vector<int>{})));
    static_assert(!noexcept(std::declval<const l07::Owner&>().snapshot()));

    check(!rvalue_view_available<l07::Owner>, "rvalue view is rejected for selected implementation");

    l07::Owner owner{1, 2, 3};
    check(copy_view(owner.view()) == std::vector<int>({1, 2, 3}), "view returns current lvalue contents");

    std::vector<int> snapshot = owner.snapshot();
    owner.replace_all({4, 5});
    check(snapshot == std::vector<int>({1, 2, 3}), "snapshot remains independent after owner mutation");
    check(copy_view(owner.view()) == std::vector<int>({4, 5}), "replace_all publishes new contents");

    const l07::Owner const_owner{7, 8};
    check(copy_view(const_owner.view()) == std::vector<int>({7, 8}), "const lvalue owner can lend read view");
}
