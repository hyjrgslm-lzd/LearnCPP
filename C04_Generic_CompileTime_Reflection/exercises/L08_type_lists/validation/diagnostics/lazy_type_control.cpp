#include "../good/type_list_tools.hpp"

#include <type_traits>

template<class T>
struct provider {
    using type = T;
};

struct explosive;

using selected = c04::lazy_type_t<true, provider<int>, explosive>;
static_assert(std::is_same_v<selected, int>);

int main() {
    return 0;
}
