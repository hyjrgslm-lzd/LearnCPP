#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace g2_common {

inline constexpr int build_cost_marker = 7;

inline int header_heavy_value(std::string_view tag) {
    std::array<int, 16> values{8, 1, 7, 2, 6, 3, 5, 4, 11, 9, 10, 12, 15, 13, 14, 16};
    std::ranges::sort(values);
    auto base = std::accumulate(values.begin(), values.end(), 0);
    std::vector<std::tuple<int, int>> pairs;
    pairs.reserve(tag.size());
    for (auto ch : tag) {
        pairs.emplace_back(static_cast<unsigned char>(ch), build_cost_marker);
    }
    return base + static_cast<int>(pairs.size());
}

}  // namespace g2_common
