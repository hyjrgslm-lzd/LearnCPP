#include <check.hpp>
#include <my_enumerate_view.hpp>

#include <ranges>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

struct MoveOnlyValue {
    int value;
    explicit MoveOnlyValue(int x) : value(x) {}
    MoveOnlyValue(MoveOnlyValue&&) = default;
    MoveOnlyValue& operator=(MoveOnlyValue&&) = default;
    MoveOnlyValue(const MoveOnlyValue&) = delete;
    MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
    bool operator==(const MoveOnlyValue&) const = default;
};

int main() {
    std::vector<int> values{10, 20, 30};
    auto enumerated = c06_g3::my_enumerate(values);

    static_assert(std::ranges::view<decltype(enumerated)>);
    static_assert(std::ranges::random_access_range<decltype(enumerated)>);
    static_assert(std::same_as<
        std::ranges::range_reference_t<decltype(enumerated)>,
        std::pair<std::ranges::range_difference_t<decltype(enumerated)>, int&>>);
    static_assert(std::same_as<
        std::ranges::range_value_t<decltype(enumerated)>,
        std::pair<std::ranges::range_difference_t<decltype(enumerated)>, int>>);

    int index = 0;
    for (auto [i, value] : enumerated) {
        check(i == index, "enumerate index follows iteration order");
        check(value == values[index], "enumerate value references the base element");
        value += 1;
        ++index;
    }
    check(values == std::vector<int>({11, 21, 31}), "proxy element writes through to base");
    check(index == 3, "enumerate visits every base element");

    auto moved = std::ranges::iter_move(enumerated.begin());
    static_assert(std::same_as<decltype(moved), std::pair<std::ptrdiff_t, int&&>>);
    check(moved.first == 0 && moved.second == 11, "iter_move returns proxy index plus element rvalue");

    auto transformed = values | c06_g3::my_enumerate() | std::views::transform([](auto&& item) {
        return static_cast<int>(item.first) + item.second; // This fixture has only three indices.
    });
    check(std::vector<int>(transformed.begin(), transformed.end()) == std::vector<int>({11, 22, 33}),
          "enumerate closure composes with transform");


    std::istringstream empty_input{""};
    auto empty_stream = c06_g3::my_enumerate(std::views::istream<int>(empty_input));
    static_assert(std::ranges::input_range<decltype(empty_stream)>);
    static_assert(!std::ranges::common_range<decltype(empty_stream)>);
    static_assert(!std::copyable<std::ranges::iterator_t<decltype(empty_stream)>>);
    check(empty_stream.begin() == empty_stream.end(), "empty non-common input range compares with sentinel");

    std::istringstream short_input{"7"};
    auto short_stream = c06_g3::my_enumerate(std::views::istream<int>(short_input));
    auto short_it = short_stream.begin();
    check((*short_it).first == 0 && (*short_it).second == 7, "short non-common input first element is readable");
    short_it++;
    check(short_it == short_stream.end(), "single-pass postfix increment reaches sentinel");

    std::istringstream input{"4 5"};
    auto stream_enum = c06_g3::my_enumerate(std::views::istream<int>(input));
    auto stream_it = stream_enum.begin();
    check((*stream_it).first == 0 && (*stream_it).second == 4, "non-common input first element is indexed");
    stream_it++;
    check((*stream_it).first == 1 && (*stream_it).second == 5, "non-common input postfix increment advances once");
    ++stream_it;
    check(stream_it == stream_enum.end(), "non-common input sentinel stops at base end");
    std::string_view text{"ok"};
    using TextEnum = decltype(c06_g3::my_enumerate(text));
    using OwningVecEnum = decltype(c06_g3::my_enumerate(std::vector<int>{}));
    check(std::ranges::borrowed_range<TextEnum>, "borrowed range is forwarded");
    check(!std::ranges::borrowed_range<OwningVecEnum>, "non-borrowed range stays non-borrowed");

    auto value_source = std::views::iota(1, 4)
        | std::views::transform([](int x) { return MoveOnlyValue{x}; });
    auto move_values = c06_g3::my_enumerate(value_source);
    auto first_value = *move_values.begin();
    auto third_value = move_values.begin()[2];
    check(first_value.first == 0 && first_value.second.value == 1,
          "enumerate forwards move-only prvalue elements");
    check(third_value.first == 2 && third_value.second.value == 3,
          "enumerate indexing forwards move-only prvalue elements");
}
