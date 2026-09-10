#pragma once

#include <concepts>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <vector>

namespace c06_f1 {

template<class Container>
struct range_of {
    using iterator = typename Container::iterator;

    Container data;

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
};

using forward_range = range_of<std::forward_list<int>>;
using bidirectional_range = range_of<std::list<int>>;
using random_access_range = range_of<std::deque<int>>;
using contiguous_range = range_of<std::vector<int>>;

struct move_only_input_iterator {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;

    int* current{};

    move_only_input_iterator() = default;
    explicit move_only_input_iterator(int* value) : current(value) {}

    move_only_input_iterator(const move_only_input_iterator&) = delete;
    move_only_input_iterator& operator=(const move_only_input_iterator&) = delete;
    move_only_input_iterator(move_only_input_iterator&&) = default;
    move_only_input_iterator& operator=(move_only_input_iterator&&) = default;

    int& operator*() const { return *current; }

    move_only_input_iterator& operator++() {
        ++current;
        return *this;
    }

    void operator++(int) { ++*this; }
};

} // namespace c06_f1
