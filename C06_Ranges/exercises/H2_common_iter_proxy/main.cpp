#include <check.hpp>
#include <common_proxy.hpp>

#include <numeric>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

namespace h2 {

void run_common_proxy_checks() {
    auto range = std::views::iota(1) | std::views::take(10);
    using iter = std::ranges::iterator_t<decltype(range)>;
    using sent = std::ranges::sentinel_t<decltype(range)>;

    my_common_iterator<iter, sent> first{range.begin()};
    my_common_iterator<iter, sent> last{range.end()};
    check(std::accumulate(first, last, 0) == 55, "common iterator feeds C++17 algorithms");

    std::vector<int> keys{1, 2, 3};
    std::vector<double> values{1.5, 2.5, 3.5};
    using zip_iter = ZipIterator<std::vector<int>::iterator, std::vector<double>::iterator>;
    zip_iter a{keys.begin(), values.begin()};
    zip_iter b{keys.begin() + 2, values.begin() + 2};

    static_assert(std::same_as<decltype(*a), std::tuple<int&, double&>>);
    static_assert(std::same_as<decltype(std::ranges::iter_move(a)), std::tuple<int&&, double&&>>);

    auto moved = std::ranges::iter_move(a);
    check(std::get<0>(moved) == 1, "iter_move reads first underlying value");
    check(std::get<1>(moved) == 1.5, "iter_move reads second underlying value");

    std::ranges::iter_swap(a, b);
    check(keys == std::vector<int>({3, 2, 1}), "iter_swap must exchange underlying elements");
    check(values == std::vector<double>({3.5, 2.5, 1.5}), "iter_swap exchanges every proxy field");
}

} // namespace h2

int main() {
    h2::run_common_proxy_checks();
}
