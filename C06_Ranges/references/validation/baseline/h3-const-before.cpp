// Frozen pre-fix compiler-negative reproduction. Never run as a normal example.
#define main c06_original_skeleton
#include "h3-before-source.txt"
#undef main
using wrapped = my_basic_const_iterator<std::vector<int>::iterator>;
static_assert(std::random_access_iterator<wrapped>, "wrapper claims random access");
int value_probe() {
    auto values = std::views::iota(0, 2)
        | std::views::transform([](int value) { return value + 1; });
    my_basic_const_iterator it(values.begin());
    return *it;
}
