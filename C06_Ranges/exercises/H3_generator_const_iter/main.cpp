#include <check.hpp>
#include <generator_const_iter.hpp>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <stdexcept>
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

} // namespace

namespace h3 {

my_generator<int> values(int count) {
    for (int i = 0; i < count; ++i) {
        co_yield i;
    }
}

my_generator<int> empty_values() {
    if (false) {
        co_yield 0;
    }
}

my_generator<int> throws_after_one() {
    co_yield 1;
    throw std::runtime_error("boom");
}

void run_generator_const_iter_checks() {
    generator_frame_destroys = 0;
    {
        auto g = values(3);
        auto it = g.begin();
        check(*it == 0, "begin resumes to first yield");
        ++it;
        check(*it == 1, "increment resumes to next yield");
        auto moved = std::move(g);
        check(g.empty(), "move construction clears source handle");
    }
    check(generator_frame_destroys == 1, "moved generator destroys frame once");

    generator_frame_destroys = 0;
    {
        auto lhs = values(1);
        auto rhs = values(2);
        lhs = std::move(rhs);
        check(generator_frame_destroys == 1, "move assignment must destroy the old coroutine frame");
        lhs = std::move(lhs);
        check(!lhs.empty(), "self move assignment keeps owned coroutine frame");
    }
    check(generator_frame_destroys == 2, "move-assigned generator destroys remaining frame");

    {
        auto g = empty_values();
        check(g.begin() == g.end(), "empty generator reaches sentinel after first resume");
    }

    {
        auto g = throws_after_one();
        auto it = g.begin();
        check(*it == 1, "generator yields before throwing");
        bool threw = false;
        try {
            ++it;
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "generator rethrows coroutine body exception on resume");
    }

    {
        std::vector<int> values{1, 2, 3};
        using wrapped = my_basic_const_iterator<std::vector<int>::iterator>;
        static_assert(std::random_access_iterator<wrapped>);
        static_assert(std::same_as<decltype(*std::declval<wrapped>()), const int&>);
        wrapped it{values.begin()};
        check(*it == 1, "const iterator reads lvalue element");
        check(it[2] == 3, "random access operator[] is implemented");
        check(*(it + 1) == 2, "random access operator+ is implemented");
    }

    {
        std::vector<bool> flags{true, false};
        using wrapped = my_basic_const_iterator<std::vector<bool>::iterator>;
        static_assert(std::same_as<decltype(*std::declval<wrapped>()), bool>);
        static_assert(!std::indirectly_writable<wrapped, bool>);
        wrapped it{flags.begin()};
        bool value = *it;
        check(value, "vector<bool> proxy is converted to non-writable bool");
    }

    {
        std::vector<int> values{4, 5};
        using moved_iter = std::move_iterator<std::vector<int>::iterator>;
        using wrapped = my_basic_const_iterator<moved_iter>;
        static_assert(std::same_as<decltype(*std::declval<wrapped>()), const int&&>);
        wrapped it{moved_iter{values.begin()}};
        int value = *it;
        check(value == 4, "move_iterator dereference keeps const rvalue category");
    }

    {
        auto source = std::views::iota(1, 4) | std::views::transform([](int x) { return x * x; });
        using wrapped = my_basic_const_iterator<std::ranges::iterator_t<decltype(source)>>;
        static_assert(std::same_as<decltype(*std::declval<wrapped>()), int>);
        wrapped it{std::ranges::begin(source)};
        int value = *it;
        check(value == 1, "prvalue dereference is returned by value");
    }

    {
        using wrapped = my_basic_const_iterator<ProxyIter>;
        static_assert(std::same_as<decltype(*std::declval<wrapped>()), const PairProxy&&>);
    }
}

} // namespace h3

int main() {
    h3::run_generator_const_iter_checks();
}
