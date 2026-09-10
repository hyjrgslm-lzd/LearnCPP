#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER)
#define C04_NOINLINE __declspec(noinline)
#else
#define C04_NOINLINE __attribute__((noinline))
#endif

namespace type_query {
template<int I>
struct tag {};

template<class T, class... Ts>
struct recursive_contains : std::false_type {};

template<class T, class Head, class... Tail>
struct recursive_contains<T, Head, Tail...>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         recursive_contains<T, Tail...>> {};

template<class T, class... Ts>
constexpr bool fold_contains = (std::is_same_v<T, Ts> || ...);

static_assert(recursive_contains<tag<3>, tag<0>, tag<1>, tag<2>, tag<3>>::value);
static_assert(fold_contains<tag<3>, tag<0>, tag<1>, tag<2>, tag<3>>);
static_assert(!recursive_contains<tag<9>, tag<0>, tag<1>, tag<2>, tag<3>>::value);
static_assert(!fold_contains<tag<9>, tag<0>, tag<1>, tag<2>, tag<3>>);
}

namespace multi_tu_contract {
template<int N>
C04_NOINLINE std::uint32_t transform(std::uint32_t value) {
    std::uint32_t result = value + static_cast<std::uint32_t>(N);
    for (int i = 0; i != 16; ++i) {
        result = (result * 33u) ^ (static_cast<std::uint32_t>(N) + static_cast<std::uint32_t>(i));
    }
    return result;
}

#undef C04_NOINLINE

extern template std::uint32_t transform<7>(std::uint32_t);
template std::uint32_t transform<7>(std::uint32_t);

std::uint32_t use_transform(std::uint32_t value) {
    return transform<7>(value);
}

std::uint32_t reference_transform(std::uint32_t value) {
    std::uint32_t result = value + 7u;
    for (int i = 0; i != 16; ++i) {
        result = (result * 33u) ^ (7u + static_cast<std::uint32_t>(i));
    }
    return result;
}
}

int main() {
    return multi_tu_contract::use_transform(3) == multi_tu_contract::reference_transform(3) ? 0 : 1;
}
