#include <generator_const_iter.hpp>

#include <concepts>
#include <iterator>
#include <type_traits>
#include <vector>

using vector_bool_iter = std::vector<bool>::iterator;
using vector_bool_const_iter = h3::my_basic_const_iterator<vector_bool_iter>;

static_assert(std::same_as<
    decltype(*std::declval<vector_bool_const_iter>()),
    std::iter_const_reference_t<vector_bool_iter>>);
static_assert(!std::indirectly_writable<vector_bool_const_iter, bool>);

using move_int_iter = std::move_iterator<std::vector<int>::iterator>;
using move_int_const_iter = h3::my_basic_const_iterator<move_int_iter>;

static_assert(std::same_as<
    decltype(*std::declval<move_int_const_iter>()),
    std::iter_const_reference_t<move_int_iter>>);

int main() {}
