#include <generator_const_iter.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

namespace {

struct PairProxy {
    int* first{};
    int* second{};
};

struct ProxyIter {
    using value_type = PairProxy;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;

    int* p{};
    PairProxy operator*() const { return {p, p + 1}; }
    ProxyIter& operator++() { ++p; return *this; }
    void operator++(int) { ++*this; }
    bool operator==(const ProxyIter&) const = default;
};

template<class I>
using standard_const_reference =
    std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>;

template<class I>
using wrapped = h3::my_basic_const_iterator<I>;

static_assert(std::same_as<
    decltype(*std::declval<wrapped<std::vector<int>::iterator>>()),
    standard_const_reference<std::vector<int>::iterator>>);
static_assert(std::same_as<
    decltype(*std::declval<wrapped<std::vector<int>::iterator>>()),
    const int&>);

static_assert(std::same_as<
    decltype(*std::declval<wrapped<std::vector<bool>::iterator>>()),
    standard_const_reference<std::vector<bool>::iterator>>);
static_assert(std::same_as<
    decltype(*std::declval<wrapped<std::vector<bool>::iterator>>()),
    bool>);
static_assert(!std::indirectly_writable<wrapped<std::vector<bool>::iterator>, bool>);

using move_int_iter = std::move_iterator<std::vector<int>::iterator>;
static_assert(std::same_as<
    decltype(*std::declval<wrapped<move_int_iter>>()),
    standard_const_reference<move_int_iter>>);
static_assert(std::same_as<
    decltype(*std::declval<wrapped<move_int_iter>>()),
    const int&&>);

using transform_source = decltype(std::views::iota(1, 4) | std::views::transform([](int x) { return x * x; }));
using transform_iter = std::ranges::iterator_t<transform_source>;
static_assert(std::same_as<
    decltype(*std::declval<wrapped<transform_iter>>()),
    standard_const_reference<transform_iter>>);
static_assert(std::same_as<
    decltype(*std::declval<wrapped<transform_iter>>()),
    int>);

static_assert(std::same_as<
    decltype(*std::declval<wrapped<ProxyIter>>()),
    standard_const_reference<ProxyIter>>);

} // namespace

int main() {}
