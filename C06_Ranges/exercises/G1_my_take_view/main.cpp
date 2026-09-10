#include <check.hpp>
#include <my_take_view.hpp>

#include <stdexcept>
#include <vector>
#include <list>
#include <ranges>

namespace {

struct short_input_range {
    int values[2]{7, 8};
    struct sentinel;
    struct iterator {
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::input_iterator_tag;
        int* current{};
        int* last{};
        int& operator*() const { return current < last ? *current : *last; }
        iterator& operator++() { if (current < last) ++current; return *this; }
        void operator++(int) { ++*this; }
        friend bool operator==(const iterator& it, sentinel);
    };
    struct sentinel {};
    iterator begin() { return iterator{values, values + 2}; }
    sentinel end() { return {}; }
};

bool operator==(const short_input_range::iterator& it, short_input_range::sentinel) {
    return it.current == it.last;
}

template<class R>
std::vector<int> collect(R&& range) {
    std::vector<int> out;
    for (int value : range) out.push_back(value);
    return out;
}

} // namespace

static_assert(std::ranges::view<c06_g1::my_take_view<std::views::all_t<std::vector<int>&>>>);

int main() {
    std::vector<int> vec{1, 2, 3, 4, 5};
    bool threw = false;
    try {
        c06_g1::my_take_view bad_count{vec, -1};
        (void)bad_count;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "negative count is rejected");

    c06_g1::my_take_view take_vec{vec, 3};
    static_assert(std::ranges::common_range<decltype(take_vec)>);
    static_assert(std::ranges::sized_range<decltype(take_vec)>);
    check(collect(take_vec) == std::vector<int>({1, 2, 3}), "vector take keeps first n values");
    check(take_vec.size() == 3, "sized range reports clamped count");
    check(take_vec.front() == 1 && take_vec.back() == 3 && take_vec[2] == 3,
          "view_interface members use the take boundary");

    c06_g1::my_take_view long_take{vec, 99};
    check(collect(long_take) == vec, "shorter-than-n vector stops at base end");
    check(long_take.size() == vec.size(), "size clamps n to the base size");

    std::list<int> list{4, 5};
    c06_g1::my_take_view take_list{list, 5};
    static_assert(!std::ranges::common_range<decltype(take_list)>);
    check(collect(take_list) == std::vector<int>({4, 5}),
          "sized non-random-access shorter-than-n stops at base end");

    short_input_range input;
    c06_g1::my_take_view take_input{input, 5};
    static_assert(!std::ranges::common_range<decltype(take_input)>);
    check(collect(take_input) == std::vector<int>({7, 8}),
          "unsized non-common input stops at count or base end");

}
