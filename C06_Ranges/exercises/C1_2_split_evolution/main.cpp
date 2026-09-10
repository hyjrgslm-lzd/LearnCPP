#include <check.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

template <std::ranges::input_range R>
std::string materialize_chars(R&& chars) {
    std::string out;
    for (char ch : chars) {
        out.push_back(ch);
    }
    return out;
}

int main() {
    std::string_view text = "hello,world,ranges";
    auto lazy_parts = text | std::views::lazy_split(',');
    auto split_parts = text | std::views::split(',');

    auto lazy_first = *lazy_parts.begin();
    auto split_first = *split_parts.begin();

    static_assert(std::ranges::forward_range<decltype(lazy_first)>);
    static_assert(!std::ranges::contiguous_range<decltype(lazy_first)>);
    static_assert(std::ranges::contiguous_range<decltype(split_first)>);

    check(materialize_chars(lazy_first) == "hello", "lazy_split subrange can be copied character by character");
    std::string_view direct{std::ranges::data(split_first), std::ranges::size(split_first)};
    check(direct == "hello", "DR split keeps contiguous subranges for contiguous input");

    check((std::ranges::to<std::vector<std::string>>(split_parts | std::views::transform([](auto part) {
               return std::string{std::ranges::data(part), std::ranges::size(part)};
           }))
           == std::vector<std::string>{"hello", "world", "ranges"}),
        "split materializes string-like fields directly");

    std::cout << "C1_2 split/lazy_split checks passed\n";
}
