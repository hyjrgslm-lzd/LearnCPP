#include <type_traits>
    #include <boost/mp11/map.hpp>
    #include <boost/mp11/list.hpp>
    template<int I> struct key {};
    template<int I> struct value { static constexpr int id = I; };

    using source_map = boost::mp11::mp_list<
        boost::mp11::mp_list<key<0>, value<0>>,
boost::mp11::mp_list<key<1>, value<1>>,
boost::mp11::mp_list<key<2>, value<2>>,
boost::mp11::mp_list<key<3>, value<3>>,
boost::mp11::mp_list<key<4>, value<4>>,
boost::mp11::mp_list<key<5>, value<5>>,
boost::mp11::mp_list<key<6>, value<6>>,
boost::mp11::mp_list<key<7>, value<7>>,
boost::mp11::mp_list<key<8>, value<8>>,
boost::mp11::mp_list<key<9>, value<9>>,
boost::mp11::mp_list<key<10>, value<10>>,
boost::mp11::mp_list<key<11>, value<11>>,
boost::mp11::mp_list<key<12>, value<12>>,
boost::mp11::mp_list<key<13>, value<13>>,
boost::mp11::mp_list<key<14>, value<14>>,
boost::mp11::mp_list<key<15>, value<15>>,
boost::mp11::mp_list<key<16>, value<16>>,
boost::mp11::mp_list<key<17>, value<17>>,
boost::mp11::mp_list<key<18>, value<18>>,
boost::mp11::mp_list<key<19>, value<19>>,
boost::mp11::mp_list<key<20>, value<20>>,
boost::mp11::mp_list<key<21>, value<21>>,
boost::mp11::mp_list<key<22>, value<22>>,
boost::mp11::mp_list<key<23>, value<23>>,
boost::mp11::mp_list<key<24>, value<24>>,
boost::mp11::mp_list<key<25>, value<25>>,
boost::mp11::mp_list<key<26>, value<26>>,
boost::mp11::mp_list<key<27>, value<27>>,
boost::mp11::mp_list<key<28>, value<28>>,
boost::mp11::mp_list<key<29>, value<29>>,
boost::mp11::mp_list<key<30>, value<30>>,
boost::mp11::mp_list<key<31>, value<31>>,
boost::mp11::mp_list<key<32>, value<32>>,
boost::mp11::mp_list<key<33>, value<33>>,
boost::mp11::mp_list<key<34>, value<34>>,
boost::mp11::mp_list<key<35>, value<35>>,
boost::mp11::mp_list<key<36>, value<36>>,
boost::mp11::mp_list<key<37>, value<37>>,
boost::mp11::mp_list<key<38>, value<38>>,
boost::mp11::mp_list<key<39>, value<39>>,
boost::mp11::mp_list<key<40>, value<40>>,
boost::mp11::mp_list<key<41>, value<41>>,
boost::mp11::mp_list<key<42>, value<42>>,
boost::mp11::mp_list<key<43>, value<43>>,
boost::mp11::mp_list<key<44>, value<44>>,
boost::mp11::mp_list<key<45>, value<45>>,
boost::mp11::mp_list<key<46>, value<46>>,
boost::mp11::mp_list<key<47>, value<47>>,
boost::mp11::mp_list<key<48>, value<48>>,
boost::mp11::mp_list<key<49>, value<49>>,
boost::mp11::mp_list<key<50>, value<50>>,
boost::mp11::mp_list<key<51>, value<51>>,
boost::mp11::mp_list<key<52>, value<52>>,
boost::mp11::mp_list<key<53>, value<53>>,
boost::mp11::mp_list<key<54>, value<54>>,
boost::mp11::mp_list<key<55>, value<55>>,
boost::mp11::mp_list<key<56>, value<56>>,
boost::mp11::mp_list<key<57>, value<57>>,
boost::mp11::mp_list<key<58>, value<58>>,
boost::mp11::mp_list<key<59>, value<59>>,
boost::mp11::mp_list<key<60>, value<60>>,
boost::mp11::mp_list<key<61>, value<61>>,
boost::mp11::mp_list<key<62>, value<62>>,
boost::mp11::mp_list<key<63>, value<63>>,
boost::mp11::mp_list<key<64>, value<64>>,
boost::mp11::mp_list<key<65>, value<65>>,
boost::mp11::mp_list<key<66>, value<66>>,
boost::mp11::mp_list<key<67>, value<67>>,
boost::mp11::mp_list<key<68>, value<68>>,
boost::mp11::mp_list<key<69>, value<69>>,
boost::mp11::mp_list<key<70>, value<70>>,
boost::mp11::mp_list<key<71>, value<71>>,
boost::mp11::mp_list<key<72>, value<72>>,
boost::mp11::mp_list<key<73>, value<73>>,
boost::mp11::mp_list<key<74>, value<74>>,
boost::mp11::mp_list<key<75>, value<75>>,
boost::mp11::mp_list<key<76>, value<76>>,
boost::mp11::mp_list<key<77>, value<77>>,
boost::mp11::mp_list<key<78>, value<78>>,
boost::mp11::mp_list<key<79>, value<79>>,
boost::mp11::mp_list<key<80>, value<80>>,
boost::mp11::mp_list<key<81>, value<81>>,
boost::mp11::mp_list<key<82>, value<82>>,
boost::mp11::mp_list<key<83>, value<83>>,
boost::mp11::mp_list<key<84>, value<84>>,
boost::mp11::mp_list<key<85>, value<85>>,
boost::mp11::mp_list<key<86>, value<86>>,
boost::mp11::mp_list<key<87>, value<87>>,
boost::mp11::mp_list<key<88>, value<88>>,
boost::mp11::mp_list<key<89>, value<89>>,
boost::mp11::mp_list<key<90>, value<90>>,
boost::mp11::mp_list<key<91>, value<91>>,
boost::mp11::mp_list<key<92>, value<92>>,
boost::mp11::mp_list<key<93>, value<93>>,
boost::mp11::mp_list<key<94>, value<94>>,
boost::mp11::mp_list<key<95>, value<95>>,
boost::mp11::mp_list<key<96>, value<96>>,
boost::mp11::mp_list<key<97>, value<97>>,
boost::mp11::mp_list<key<98>, value<98>>,
boost::mp11::mp_list<key<99>, value<99>>,
boost::mp11::mp_list<key<100>, value<100>>,
boost::mp11::mp_list<key<101>, value<101>>,
boost::mp11::mp_list<key<102>, value<102>>,
boost::mp11::mp_list<key<103>, value<103>>,
boost::mp11::mp_list<key<104>, value<104>>,
boost::mp11::mp_list<key<105>, value<105>>,
boost::mp11::mp_list<key<106>, value<106>>,
boost::mp11::mp_list<key<107>, value<107>>,
boost::mp11::mp_list<key<108>, value<108>>,
boost::mp11::mp_list<key<109>, value<109>>,
boost::mp11::mp_list<key<110>, value<110>>,
boost::mp11::mp_list<key<111>, value<111>>,
boost::mp11::mp_list<key<112>, value<112>>,
boost::mp11::mp_list<key<113>, value<113>>,
boost::mp11::mp_list<key<114>, value<114>>,
boost::mp11::mp_list<key<115>, value<115>>,
boost::mp11::mp_list<key<116>, value<116>>,
boost::mp11::mp_list<key<117>, value<117>>,
boost::mp11::mp_list<key<118>, value<118>>,
boost::mp11::mp_list<key<119>, value<119>>,
boost::mp11::mp_list<key<120>, value<120>>,
boost::mp11::mp_list<key<121>, value<121>>,
boost::mp11::mp_list<key<122>, value<122>>,
boost::mp11::mp_list<key<123>, value<123>>,
boost::mp11::mp_list<key<124>, value<124>>,
boost::mp11::mp_list<key<125>, value<125>>,
boost::mp11::mp_list<key<126>, value<126>>,
boost::mp11::mp_list<key<127>, value<127>>>;

    template<class T>
    struct entry_key;

    template<template<class...> class L, class K, class V, class... Rest>
    struct entry_key<L<K, V, Rest...>> : std::type_identity<K> {};

    template<class T>
    struct entry_value : std::type_identity<void> {};

    template<template<class...> class L, class K, class V, class... Rest>
    struct entry_value<L<K, V, Rest...>> : std::type_identity<V> {};

    template<class M, class K>
    struct manual_find;

    template<class K>
    struct manual_find<boost::mp11::mp_list<>, K> : std::type_identity<void> {};

    template<class Head, class... Tail, class K>
    struct manual_find<boost::mp11::mp_list<Head, Tail...>, K>
        : std::conditional_t<std::is_same_v<typename entry_key<Head>::type, K>,
                             std::type_identity<Head>,
                             manual_find<boost::mp11::mp_list<Tail...>, K>> {};

        using entry = typename manual_find<source_map, key<128>>::type;

        using answer = typename entry_value<entry>::type;
        static_assert(std::is_same_v<answer, void>);
        extern "C" __declspec(dllexport) int result() {
            return -1;
        }
        int main() {
            return result() == -1 ? 0 : 1;
        }
