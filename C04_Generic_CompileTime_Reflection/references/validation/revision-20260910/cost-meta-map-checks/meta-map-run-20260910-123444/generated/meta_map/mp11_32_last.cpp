#include <type_traits>
template<int I> struct key {};
template<int I> struct value { static constexpr int id = I; };


#include <boost/mp11/map.hpp>
#include <boost/mp11/list.hpp>
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
    boost::mp11::mp_list<key<31>, value<31>>>;
using entry = boost::mp11::mp_map_find<source_map, key<31>>;

template<class T>
struct entry_value : std::type_identity<void> {};

template<template<class...> class L, class K, class V, class... Rest>
struct entry_value<L<K, V, Rest...>> : std::type_identity<V> {};

using answer = typename entry_value<entry>::type;

        static_assert(std::is_same_v<answer, value<31>>);
        extern "C" __declspec(dllexport) int result() {
            if constexpr (std::is_void_v<answer>) {
                return -1;
            } else {
                return answer::id;
            }
        }
        int main() {
            return result() == 31 ? 0 : 1;
        }
