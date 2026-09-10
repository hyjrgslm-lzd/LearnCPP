#pragma once

#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <vector>

namespace c06_f1 {

template<class Tag>
struct ptr_iterator {
    using value_type = int;
    using reference = int&;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::conditional_t<std::same_as<Tag, std::contiguous_iterator_tag>, std::random_access_iterator_tag, Tag>;
    using iterator_concept = Tag;
    int* p{};
    ptr_iterator() = default;
    explicit ptr_iterator(int* value) : p(value) {}
    int& operator*() const { return *p; }
    int* operator->() const requires std::same_as<Tag, std::contiguous_iterator_tag> { return p; }
    int& operator[](difference_type n) const requires std::derived_from<Tag, std::random_access_iterator_tag> { return p[n]; }
    ptr_iterator& operator++() { ++p; return *this; }
    ptr_iterator operator++(int) { auto old = *this; ++*this; return old; }
    ptr_iterator& operator--() requires std::derived_from<Tag, std::bidirectional_iterator_tag> { --p; return *this; }
    ptr_iterator operator--(int) requires std::derived_from<Tag, std::bidirectional_iterator_tag> { auto old = *this; --*this; return old; }
    ptr_iterator& operator+=(difference_type n) requires std::derived_from<Tag, std::random_access_iterator_tag> { p += n; return *this; }
    ptr_iterator& operator-=(difference_type n) requires std::derived_from<Tag, std::random_access_iterator_tag> { p -= n; return *this; }
    friend ptr_iterator operator+(ptr_iterator it, difference_type n) requires std::derived_from<Tag, std::random_access_iterator_tag> { it += n; return it; }
    friend ptr_iterator operator+(difference_type n, ptr_iterator it) requires std::derived_from<Tag, std::random_access_iterator_tag> { it += n; return it; }
    friend ptr_iterator operator-(ptr_iterator it, difference_type n) requires std::derived_from<Tag, std::random_access_iterator_tag> { it -= n; return it; }
    friend difference_type operator-(ptr_iterator a, ptr_iterator b) requires std::derived_from<Tag, std::random_access_iterator_tag> {
        return a.p - b.p + 1;
    }
    auto operator<=>(const ptr_iterator&) const requires std::derived_from<Tag, std::random_access_iterator_tag> = default;
    bool operator==(const ptr_iterator&) const = default;
};

template<class It>
struct range_of {
    using iterator = It;
    std::vector<int> data;
    iterator begin() { return iterator{data.data()}; }
    iterator end() { return iterator{data.data() + data.size()}; }
};

using forward_range = range_of<ptr_iterator<std::forward_iterator_tag>>;
using bidirectional_range = range_of<ptr_iterator<std::bidirectional_iterator_tag>>;
using random_access_range = range_of<ptr_iterator<std::random_access_iterator_tag>>;
using contiguous_range = range_of<ptr_iterator<std::contiguous_iterator_tag>>;

struct move_only_input_iterator {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;
    int* ptr{};
    move_only_input_iterator() = default;
    explicit move_only_input_iterator(int* p) : ptr(p) {}
    move_only_input_iterator(const move_only_input_iterator&) = delete;
    move_only_input_iterator& operator=(const move_only_input_iterator&) = delete;
    move_only_input_iterator(move_only_input_iterator&&) = default;
    move_only_input_iterator& operator=(move_only_input_iterator&&) = default;
    int& operator*() const { return *ptr; }
    move_only_input_iterator& operator++() { ++ptr; return *this; }
    void operator++(int) { ++*this; }
    bool operator==(const move_only_input_iterator&) const = default;
};

} // namespace c06_f1
