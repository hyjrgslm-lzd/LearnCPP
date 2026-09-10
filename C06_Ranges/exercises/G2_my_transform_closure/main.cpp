#include <check.hpp>
#include <my_transform_view.hpp>

#include <memory>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <vector>

namespace {

struct MoveOnlyAdd {
    std::unique_ptr<int> delta;
    explicit MoveOnlyAdd(int value) : delta(std::make_unique<int>(value)) {}
    MoveOnlyAdd(MoveOnlyAdd&&) = default;
    MoveOnlyAdd& operator=(MoveOnlyAdd&&) = default;
    MoveOnlyAdd(const MoveOnlyAdd&) = delete;
    MoveOnlyAdd& operator=(const MoveOnlyAdd&) = delete;
    int operator()(int value) { return value + *delta; }
};

struct Stateful {
    int delta{};
    int* copies{};
    Stateful(int value, int& observed_copies) : delta(value), copies(&observed_copies) {}
    Stateful(const Stateful& other) : delta(other.delta), copies(other.copies) { ++*copies; }
    Stateful(Stateful&&) = default;
    Stateful& operator=(const Stateful&) = default;
    Stateful& operator=(Stateful&&) = default;
    int operator()(int value) const { return value + delta; }
};

template<class R>
std::vector<int> collect(R&& range) {
    std::vector<int> out;
    for (int value : range) out.push_back(value);
    return out;
}

} // namespace

int main() {
    std::vector<int> values{1, 2, 3};
    auto square = [](int value) { return value * value; };
    auto inc = [](int value) { return value + 1; };

    auto squared = values | c06_g2::my_transform(square);
    static_assert(std::ranges::view<decltype(squared)>);
    static_assert(std::ranges::random_access_range<decltype(squared)>);
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(squared)>, int>);
    check(collect(squared) == std::vector<int>({1, 4, 9}), "prvalue transform produces expected values");

    auto composed = values | (c06_g2::my_transform(square) | c06_g2::my_transform(inc));
    check(collect(composed) == std::vector<int>({2, 5, 10}), "closure composition applies left then right");

    auto move_only = values | c06_g2::my_transform(MoveOnlyAdd{5});
    check(collect(move_only) == std::vector<int>({6, 7, 8}), "move-only callable is accepted by rvalue closure");


    std::istringstream empty_input{""};
    auto empty_stream = c06_g2::my_transform(std::views::istream<int>(empty_input), [](int value) { return value + 1; });
    static_assert(std::ranges::input_range<decltype(empty_stream)>);
    static_assert(!std::ranges::common_range<decltype(empty_stream)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(empty_stream)>>);
    check(empty_stream.begin() == empty_stream.end(), "empty non-common input transform compares with sentinel");

    std::istringstream short_input{"7"};
    auto short_stream = c06_g2::my_transform(std::views::istream<int>(short_input), [](int value) { return value * 2; });
    auto short_it = short_stream.begin();
    check(*short_it == 14, "short non-common input transform reads first value");
    short_it++;
    check(short_it == short_stream.end(), "single-pass postfix increment reaches transform sentinel");

    std::istringstream input{"4 5"};
    auto stream_transform = c06_g2::my_transform(std::views::istream<int>(input), [](int value) { return value + 10; });
    auto stream_it = stream_transform.begin();
    check(*stream_it == 14, "non-common input transform first element is transformed");
    stream_it++;
    check(*stream_it == 15, "non-common input transform postfix increment advances once");
    ++stream_it;
    check(stream_it == stream_transform.end(), "non-common input transform sentinel stops at base end");
    int copies = 0;
    Stateful state{10, copies};
    auto direct = c06_g2::my_transform(values, state);
    state.delta = 1000;
    const int copies_before_iteration = copies;
    auto first = collect(direct);
    check(first == std::vector<int>({11, 12, 13}), "stateful callable is stored by value");
    check(copies == copies_before_iteration, "dereference does not copy the callable");

    auto second = collect(direct);
    check(second == first, "repeated traversal preserves pure callable results");
    check(copies == copies_before_iteration, "repeated traversal does not copy the callable");
}
