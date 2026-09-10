#include <check.hpp>
#include <iterator_hierarchy.hpp>

#include <concepts>
#include <iterator>
#include <ranges>
#include <vector>

namespace {

template<std::ranges::input_range R>
std::vector<int> collect(R& range) {
    std::vector<int> out;
    for (int value : range) out.push_back(value);
    return out;
}

} // namespace

using forward_it = std::ranges::iterator_t<c06_f1::forward_range>;
using bidirectional_it = std::ranges::iterator_t<c06_f1::bidirectional_range>;
using random_access_it = std::ranges::iterator_t<c06_f1::random_access_range>;
using contiguous_it = std::ranges::iterator_t<c06_f1::contiguous_range>;

static_assert(std::forward_iterator<forward_it>);
static_assert(std::input_iterator<forward_it>);
static_assert(!std::bidirectional_iterator<forward_it>);
static_assert(std::copyable<forward_it>);

static_assert(std::bidirectional_iterator<bidirectional_it>);
static_assert(std::forward_iterator<bidirectional_it>);

static_assert(std::random_access_iterator<random_access_it>);
static_assert(std::bidirectional_iterator<random_access_it>);

static_assert(std::contiguous_iterator<contiguous_it>);
static_assert(std::random_access_iterator<contiguous_it>);
static_assert(std::same_as<
    std::iterator_traits<contiguous_it>::iterator_category,
    std::random_access_iterator_tag>);

static_assert(std::input_iterator<c06_f1::move_only_input_iterator>);
static_assert(!std::forward_iterator<c06_f1::move_only_input_iterator>);
static_assert(!std::copyable<c06_f1::move_only_input_iterator>);

int main() {
    c06_f1::forward_range fwd{{1, 2, 3, 4, 5}};
    check(collect(fwd) == std::vector<int>({1, 2, 3, 4, 5}),
          "forward traversal preserves order");

    c06_f1::bidirectional_range bidir{{1, 2, 3, 4, 5}};
    std::vector<int> reversed;
    auto it = bidir.end();
    while (it != bidir.begin()) {
        --it;
        reversed.push_back(*it);
    }
    check(reversed == std::vector<int>({5, 4, 3, 2, 1}),
          "bidirectional iterator moves backward");

    c06_f1::random_access_range ra{{1, 2, 3, 4, 5}};
    check(ra.begin()[2] == 3, "random access subscript reads the third element");
    check(ra.end() - ra.begin() == 5, "random access distance matches");
    check(*(ra.begin() + 4) == 5, "random access addition reaches the last element");

    c06_f1::contiguous_range contiguous{{1, 2, 3}};
    check(std::to_address(contiguous.begin()) == contiguous.data.data(),
          "contiguous iterator exposes the underlying address");

    int input[]{7, 8};
    c06_f1::move_only_input_iterator first{input};
    check(*first == 7, "move-only input iterator dereferences current element");
    ++first;
    check(*first == 8, "move-only input iterator advances once");
}
