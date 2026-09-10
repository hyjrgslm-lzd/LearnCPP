#include <check.hpp>
#include <filter_cache.hpp>

#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

namespace {

struct CountingEven {
    int* calls{};

    bool operator()(int value) const {
        ++*calls;
        return value % 2 == 0;
    }
};

template<class T>
concept const_beginable = requires(const T& value) {
    value.begin();
};

} // namespace

namespace h1 {

void run_filter_cache_checks() {
    std::vector<int> values{1, 3, 4, 6, 7, 10};
    int calls = 0;
    auto view = my_filter_view(std::views::all(values), CountingEven{&calls});

    auto first = view.begin();
    check(*first == 4, "first begin finds first accepted value");
    check(calls == 3, "first begin scans until accepted value");

    auto again = view.begin();
    check(*again == 4, "cached begin returns same accepted value");
    check(calls == 3, "second begin must use cached iterator");

    auto copy = view;
    auto copy_first = copy.begin();
    check(*copy_first == 4, "copy still finds first accepted value");
    check(calls == 6, "copy must rescan after non-propagating cache reset");

    std::vector<int> seen;
    for (int value : copy) {
        seen.push_back(value);
    }
    check((seen == std::vector<int>{4, 6, 10}), "filter iterator skips rejected values");

    static_assert(!const_beginable<decltype(view)>,
        "forward cached filter begin is non-const; C++26 input-only const branch is separate");
}

} // namespace h1

int main() {
    h1::run_filter_cache_checks();
}
