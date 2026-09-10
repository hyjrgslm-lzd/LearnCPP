#include <check.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

int main() {
    std::vector<int> data{1, 1, 2, 2, 2, 3};
    auto groups = data | std::views::chunk_by(std::equal_to<>{});

    std::vector<int> sizes;
    for (auto group : groups) {
        sizes.push_back(static_cast<int>(std::ranges::distance(group)));
    }
    check((sizes == std::vector<int>{2, 3, 1}), "chunk_by groups adjacent equal runs");

    std::vector<int> rising{1, 2, 3, 1, 2, 4, 5};
    auto monotonic_groups = rising | std::views::chunk_by([](int left, int right) { return left <= right; });
    std::vector<int> monotonic_sizes;
    for (auto group : monotonic_groups) {
        monotonic_sizes.push_back(static_cast<int>(std::ranges::distance(group)));
    }
    check((monotonic_sizes == std::vector<int>{3, 4}), "chunk_by can group adjacent nondecreasing runs");

    auto rle = data | std::views::chunk_by(std::equal_to<>{})
        | std::views::transform([](auto group) {
              return std::pair{*group.begin(), static_cast<int>(std::ranges::distance(group))};
          })
        | std::ranges::to<std::vector>();
    check(rle.size() == 3 && rle[1].first == 2 && rle[1].second == 3, "chunk_by feeds run-length encoding");

    std::vector<std::vector<char>> words{{'a', 'b'}, {'c'}, {'d', 'e'}};
    auto joined = words | std::views::join_with('-') | std::ranges::to<std::string>();
    check(joined == "ab-c-de", "join_with inserts a delimiter between ranges");

    auto readonly = data | std::views::as_const;
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(readonly)>, const int&>);
    check(*readonly.begin() == 1, "as_const changes element access, not object lifetime");

    std::vector<int> editable{10, 20, 30};
    int copied = 0;
    for (auto [dst, src] : std::views::zip(editable, data | std::views::as_const)) {
        dst += src;
        copied += src;
    }
    check((editable == std::vector<int>{11, 21, 32}), "zip can pair writable and readonly sides");
    check(copied == 4, "zip stops at the shortest side");

    std::vector<std::string> words_to_move{"aa", "bb"};
    auto moved = words_to_move | std::views::as_rvalue | std::ranges::to<std::vector<std::string>>();
    check((moved == std::vector<std::string>{"aa", "bb"}), "as_rvalue lets ranges::to move elements");
    check(words_to_move.size() == 2, "moved-from source container still owns two valid string objects");

    std::cout << "D2 chunk_by/join_with/as_const/as_rvalue checks passed\n";
}
