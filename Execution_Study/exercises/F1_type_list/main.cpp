// ============================================================
// Exercise F-1: Type list infrastructure
// ============================================================
// Goal: Implement type_list<Ts...> and compile-time operations
//       (concat, unique, transform, filter).  All verification
//       is via static_assert -- if this compiles, all tests pass.
// ============================================================

#include <type_traits>
#include <iostream>

// ============================================================
// type_list definition
// ============================================================
template <typename... Ts>
struct type_list {};

// ============================================================
// concat
// ============================================================
template <typename L1, typename L2>
struct concat_impl;

template <typename... Ts, typename... Us>
struct concat_impl<type_list<Ts...>, type_list<Us...>> {
    using type = type_list<Ts..., Us...>;
};

template <typename L1, typename L2>
using concat = typename concat_impl<L1, L2>::type;

// ---- concat verification ----
static_assert(std::is_same_v<
    concat<type_list<int, float>, type_list<double>>,
    type_list<int, float, double>
>);

// ============================================================
// contains (helper)
// ============================================================
// TODO [必做]: implement contains<TypeList, T>
//   Should be a bool compile-time constant: contains_v<List, T>.
//
//   Hint -- recursive approach:
//     Base case: contains<type_list<>, T> = false
//     Recursive: contains<type_list<Head, Tail...>, T>
//                = is_same_v<Head, T> || contains<type_list<Tail...>, T>
//
// template <typename List, typename T>
// struct contains_impl;
//
// template <typename T>
// struct contains_impl<type_list<>, T> : std::false_type {};
//
// template <typename Head, typename... Tail, typename T>
// struct contains_impl<type_list<Head, Tail...>, T>
//     : std::bool_constant<std::is_same_v<Head, T>
//                           || contains_impl<type_list<Tail...>, T>::value> {};
//
// template <typename List, typename T>
// inline constexpr bool contains_v = contains_impl<List, T>::value;

// ============================================================
// unique
// ============================================================
// TODO [必做]: implement unique<TypeList>
//   Removes duplicate types, preserving first occurrence order.
//
//   Hint -- accumulate approach:
//     unique_impl<InputList, AccumulatedResult>
//     Base case: unique_impl<type_list<>, Acc> -> Acc
//     Recursive: if Head is already in Acc, skip it; else append it.
//
// template <typename Input, typename Acc = type_list<>>
// struct unique_impl;
//
// template <typename Acc>
// struct unique_impl<type_list<>, Acc> { using type = Acc; };
//
// template <typename Head, typename... Tail, typename Acc>
// struct unique_impl<type_list<Head, Tail...>, Acc> {
//     using type = std::conditional_t<
//         contains_v<Acc, Head>,
//         typename unique_impl<type_list<Tail...>, Acc>::type,
//         typename unique_impl<type_list<Tail...>, concat<Acc, type_list<Head>>>::type
//     >;
// };
//
// template <typename List>
// using unique = typename unique_impl<List>::type;

// ---- unique verification ----
// TODO [必做]: uncomment after implementing unique
// static_assert(std::is_same_v<
//     unique<type_list<int, float, int, double, float>>,
//     type_list<int, float, double>
// >);

// ============================================================
// transform
// ============================================================
// TODO [必做]: implement transform<TypeList, MetaFn>
//   Applies MetaFn to each type in the list.
//   MetaFn is a template with a nested `type` alias:
//     template <typename T> struct MetaFn { using type = ...; };
//
// template <typename List, template <typename> class MetaFn>
// struct transform_impl;
//
// template <typename... Ts, template <typename> class MetaFn>
// struct transform_impl<type_list<Ts...>, MetaFn> {
//     using type = type_list<typename MetaFn<Ts>::type...>;
// };
//
// template <typename List, template <typename> class MetaFn>
// using transform = typename transform_impl<List, MetaFn>::type;

// ---- transform helper: add_pointer ----
// TODO [必做]: define add_pointer metafunction
// template <typename T>
// struct add_pointer { using type = T*; };

// ---- transform verification ----
// TODO [必做]: uncomment after implementing transform + add_pointer
// static_assert(std::is_same_v<
//     transform<type_list<int, float>, add_pointer>,
//     type_list<int*, float*>
// >);

// ============================================================
// filter
// ============================================================
// TODO [必做]: implement filter<TypeList, Pred>
//   Keeps only types for which Pred<T>::value is true.
//   Pred is a template with a static constexpr bool value,
//   or inheriting from std::true_type / std::false_type.
//
// template <typename List, template <typename> class Pred>
// struct filter_impl;
//
// template <template <typename> class Pred>
// struct filter_impl<type_list<>, Pred> {
//     using type = type_list<>;
// };
//
// template <typename Head, typename... Tail, template <typename> class Pred>
// struct filter_impl<type_list<Head, Tail...>, Pred> {
//     using rest = typename filter_impl<type_list<Tail...>, Pred>::type;
//     using type = std::conditional_t<
//         Pred<Head>::value,
//         concat<type_list<Head>, rest>,
//         rest
//     >;
// };
//
// template <typename List, template <typename> class Pred>
// using filter = typename filter_impl<List, Pred>::type;

// ---- filter helper: is_integral_pred ----
// template <typename T>
// struct is_integral_pred : std::is_integral<T> {};

// ---- filter verification ----
// TODO [必做]: uncomment after implementing filter
// static_assert(std::is_same_v<
//     filter<type_list<int, float, long, double>, is_integral_pred>,
//     type_list<int, long>
// >);

// ============================================================
// when_all simulation
// ============================================================
// Sender A's value types: type_list<int>
// Sender B's value types: type_list<float, std::string>
// when_all result: concat -> type_list<int, float, std::string>
//
// TODO [必做]: concat sender A's types with sender B's types
//   and verify with static_assert.
//
// #include <string>
// using sender_a_values = type_list<int>;
// using sender_b_values = type_list<float, std::string>;
// using when_all_values = concat<sender_a_values, sender_b_values>;
//
// static_assert(std::is_same_v<
//     when_all_values,
//     type_list<int, float, std::string>
// >);

// ============================================================
// TODO [进阶]: implement size<TypeList> compile-time constant
// TODO [进阶]: implement at<TypeList, N> compile-time indexing
// TODO [进阶]: implement flatten<TypeListOfTypeLists>
// ============================================================

int main() {
    // All verification is at compile time via static_assert.
    // If this compiles, all tests pass.
    std::cout << "All type_list static_asserts passed!\n";
    return 0;
}
