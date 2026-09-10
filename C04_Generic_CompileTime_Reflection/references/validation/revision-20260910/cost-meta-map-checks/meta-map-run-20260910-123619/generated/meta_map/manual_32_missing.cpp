#include <type_traits>
template<int I> struct key {};
template<int I> struct value { static constexpr int id = I; };


template<class K, class V>
struct pair {
    using key_type = K;
    using value_type = V;
};

template<class K, class... Pairs>
struct map_find : std::type_identity<void> {};

template<class K, class Head, class... Tail>
struct map_find<K, Head, Tail...>
    : std::conditional_t<std::is_same_v<typename Head::key_type, K>,
                         std::type_identity<typename Head::value_type>,
                         map_find<K, Tail...>> {};

using answer = typename map_find<key<32>,
    pair<key<0>, value<0>>,
    pair<key<1>, value<1>>,
    pair<key<2>, value<2>>,
    pair<key<3>, value<3>>,
    pair<key<4>, value<4>>,
    pair<key<5>, value<5>>,
    pair<key<6>, value<6>>,
    pair<key<7>, value<7>>,
    pair<key<8>, value<8>>,
    pair<key<9>, value<9>>,
    pair<key<10>, value<10>>,
    pair<key<11>, value<11>>,
    pair<key<12>, value<12>>,
    pair<key<13>, value<13>>,
    pair<key<14>, value<14>>,
    pair<key<15>, value<15>>,
    pair<key<16>, value<16>>,
    pair<key<17>, value<17>>,
    pair<key<18>, value<18>>,
    pair<key<19>, value<19>>,
    pair<key<20>, value<20>>,
    pair<key<21>, value<21>>,
    pair<key<22>, value<22>>,
    pair<key<23>, value<23>>,
    pair<key<24>, value<24>>,
    pair<key<25>, value<25>>,
    pair<key<26>, value<26>>,
    pair<key<27>, value<27>>,
    pair<key<28>, value<28>>,
    pair<key<29>, value<29>>,
    pair<key<30>, value<30>>,
    pair<key<31>, value<31>>>::type;

        static_assert(std::is_same_v<answer, void>);
        extern "C" __declspec(dllexport) int result() {
            return -1;
        }
        int main() {
            return result() == -1 ? 0 : 1;
        }
