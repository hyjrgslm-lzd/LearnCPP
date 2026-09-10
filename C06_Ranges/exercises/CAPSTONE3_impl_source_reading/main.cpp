#include <check.hpp>

#include <algorithm>
#include <ranges>
#include <iterator>
#include <type_traits>
#include <vector>
#include <cstdio>

template<class T>
concept const_beginable = requires(const T& value) {
    value.begin();
};

static_assert(std::is_same_v<
    std::ranges::iterator_t<std::ranges::single_view<int>>,
    int*
>);

// 1b. iota_view<int,int> 是 borrowed_range（迭代器不依赖 view 生命周期）
static_assert(std::ranges::borrowed_range<std::ranges::iota_view<int,int>>);

using iota_iter = std::ranges::iterator_t<std::ranges::iota_view<int,int>>;
static_assert(std::is_same_v<
    typename iota_iter::iterator_concept,
    std::random_access_iterator_tag
>);

// 1c. empty_view 是 sized_range 和 borrowed_range；size/distance 均为 0。
static_assert(std::ranges::sized_range<std::ranges::empty_view<int>>);
static_assert(std::ranges::borrowed_range<std::ranges::empty_view<int>>);

int main()
{
    auto tv = std::vector<int>{1, 2, 3} | std::views::transform([](int x) { return x * x; });
    using tv_iter = std::ranges::iterator_t<decltype(tv)>;
    static_assert(std::is_same_v<typename tv_iter::iterator_concept, std::random_access_iterator_tag>);

    auto fv = std::vector<int>{1, 2, 3, 4, 5} | std::views::filter([](int x) { return x % 2 == 0; });
    using fv_iter = std::ranges::iterator_t<decltype(fv)>;
    static_assert(std::is_same_v<typename fv_iter::iterator_concept, std::bidirectional_iterator_tag>);
    static_assert(!std::ranges::sized_range<decltype(fv)>);
    static_assert(!const_beginable<decltype(fv)>);

    auto jv = std::vector<std::vector<int>>{{1, 2}, {3, 4}} | std::views::join;
    static_assert(!std::ranges::borrowed_range<decltype(jv)>);
    using jv_iter = std::ranges::iterator_t<decltype(jv)>;
    static_assert(std::bidirectional_iterator<jv_iter> || std::forward_iterator<jv_iter>);

    auto begin_fn = std::ranges::begin;
    check(!std::is_function_v<decltype(begin_fn)>, "ranges begin is a CPO object");
    check(std::ranges::distance(std::ranges::empty_view<int>{}) == 0, "empty_view is a zero-sized range");
    check(std::ranges::distance(jv) == 4, "join_view flattens nested ranges");
    std::puts("CAPSTONE3 implementation-source observations OK");
    return 0;
}
