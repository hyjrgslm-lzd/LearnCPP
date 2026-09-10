#include <type_traits>
        template<int I> struct tag {};

template<class T, class... Ts>
constexpr bool contains = (std::is_same_v<T, Ts> || ...);

constexpr bool answer = contains<tag<31>, tag<0>, tag<1>, tag<2>, tag<3>, tag<4>, tag<5>, tag<6>, tag<7>, tag<8>, tag<9>, tag<10>, tag<11>, tag<12>, tag<13>, tag<14>, tag<15>, tag<16>, tag<17>, tag<18>, tag<19>, tag<20>, tag<21>, tag<22>, tag<23>, tag<24>, tag<25>, tag<26>, tag<27>, tag<28>, tag<29>, tag<30>, tag<31>>;
constexpr bool missing = contains<tag<32>, tag<0>, tag<1>, tag<2>, tag<3>, tag<4>, tag<5>, tag<6>, tag<7>, tag<8>, tag<9>, tag<10>, tag<11>, tag<12>, tag<13>, tag<14>, tag<15>, tag<16>, tag<17>, tag<18>, tag<19>, tag<20>, tag<21>, tag<22>, tag<23>, tag<24>, tag<25>, tag<26>, tag<27>, tag<28>, tag<29>, tag<30>, tag<31>>;
constexpr bool empty = contains<tag<31>>;

        static_assert(answer);
        static_assert(!missing);
        static_assert(!empty);
        extern "C" __declspec(dllexport) int result() { return (answer ? 100 : 0) + (missing ? 10 : 0) + (empty ? 1 : 0); }
        int main() { return result() == 100 ? 0 : 1; }
