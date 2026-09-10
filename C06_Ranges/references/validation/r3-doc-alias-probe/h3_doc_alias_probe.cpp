#include <concepts>
#include <iterator>
#include <type_traits>
#include <vector>

template<class I>
using doc_iter_const_reference_t =
    std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>;

static_assert(std::same_as<doc_iter_const_reference_t<std::vector<int>::iterator>, const int&>);
static_assert(std::same_as<doc_iter_const_reference_t<std::vector<bool>::iterator>, bool>);
static_assert(std::same_as<doc_iter_const_reference_t<std::move_iterator<std::vector<int>::iterator>>, const int&&>);

struct tiny_const_iterator {
    std::vector<bool>::iterator it;
    using value_type = std::iter_value_t<std::vector<bool>::iterator>;
    using difference_type = std::iter_difference_t<std::vector<bool>::iterator>;
    using reference = doc_iter_const_reference_t<std::vector<bool>::iterator>;
    reference operator*() const { return *it; }
    tiny_const_iterator& operator++() { ++it; return *this; }
    tiny_const_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
    bool operator==(const tiny_const_iterator&) const = default;
};

static_assert(!std::indirectly_writable<tiny_const_iterator, bool>);

int main() {}
