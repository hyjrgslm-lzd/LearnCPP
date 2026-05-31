#include <type_traits>
#include <string>
#include <exception>

// ============================================================
// Tag types representing the three completion channels
// ============================================================

template <typename... Args>
struct set_value_t {};

template <typename Err>
struct set_error_t {};

struct set_stopped_t {};

// ============================================================
// completion_signatures<Sigs...>
// ============================================================

template <typename... Sigs>
struct completion_signatures {};

// ============================================================
// type_list helper (from F-1)
// ============================================================

template <typename... Ts>
struct type_list {};

// contains
template <typename List, typename T>
struct contains_impl;

template <typename T>
struct contains_impl<type_list<>, T> : std::false_type {};

template <typename Head, typename... Tail, typename T>
struct contains_impl<type_list<Head, Tail...>, T>
    : std::conditional_t<std::is_same_v<Head, T>,
                         std::true_type,
                         contains_impl<type_list<Tail...>, T>> {};

template <typename List, typename T>
inline constexpr bool contains_v = contains_impl<List, T>::value;

// concat
template <typename L1, typename L2>
struct concat_impl;

template <typename... Ts, typename... Us>
struct concat_impl<type_list<Ts...>, type_list<Us...>> {
    using type = type_list<Ts..., Us...>;
};

template <typename L1, typename L2>
using concat = typename concat_impl<L1, L2>::type;

// filter
template <typename List, template <typename> class Pred, typename Acc = type_list<>>
struct filter_impl;

template <template <typename> class Pred, typename Acc>
struct filter_impl<type_list<>, Pred, Acc> {
    using type = Acc;
};

template <typename Head, typename... Tail, template <typename> class Pred, typename Acc>
struct filter_impl<type_list<Head, Tail...>, Pred, Acc>
    : std::conditional_t<
          Pred<Head>::value,
          filter_impl<type_list<Tail...>, Pred, concat<Acc, type_list<Head>>>,
          filter_impl<type_list<Tail...>, Pred, Acc>> {};

template <typename List, template <typename> class Pred>
using filter = typename filter_impl<List, Pred>::type;

// transform
template <typename List, template <typename> class MetaFn>
struct transform_impl;

template <template <typename> class MetaFn>
struct transform_impl<type_list<>, MetaFn> {
    using type = type_list<>;
};

template <typename Head, typename... Tail, template <typename> class MetaFn>
struct transform_impl<type_list<Head, Tail...>, MetaFn> {
    using type = concat<type_list<typename MetaFn<Head>::type>,
                        typename transform_impl<type_list<Tail...>, MetaFn>::type>;
};

template <typename List, template <typename> class MetaFn>
using transform = typename transform_impl<List, MetaFn>::type;

// ============================================================
// Predicates: is_set_value, is_set_error, is_set_stopped
// ============================================================

template <typename T>
struct is_set_value : std::false_type {};

template <typename... Args>
struct is_set_value<set_value_t<Args...>> : std::true_type {};

template <typename T>
struct is_set_error : std::false_type {};

template <typename Err>
struct is_set_error<set_error_t<Err>> : std::true_type {};

template <typename T>
struct is_set_stopped : std::false_type {};

template <>
struct is_set_stopped<set_stopped_t> : std::true_type {};

// ============================================================
// Helper: convert type_list to completion_signatures and vice versa
// ============================================================

template <typename Sigs>
struct sigs_to_list;

template <typename... Ss>
struct sigs_to_list<completion_signatures<Ss...>> {
    using type = type_list<Ss...>;
};

template <typename Sigs>
using sigs_to_list_t = typename sigs_to_list<Sigs>::type;

template <typename List>
struct list_to_sigs;

template <typename... Ts>
struct list_to_sigs<type_list<Ts...>> {
    using type = completion_signatures<Ts...>;
};

template <typename List>
using list_to_sigs_t = typename list_to_sigs<List>::type;

// ============================================================
// TODO [必做]: implement value_types_of<Sigs>
// Extract all set_value_t<...> signatures from completion_signatures.
// Input:  completion_signatures<set_value_t<int>, set_error_t<std::exception_ptr>, set_stopped_t>
// Output: type_list<set_value_t<int>>
//
// Hint: use sigs_to_list_t to convert to type_list, then filter with is_set_value.
// ============================================================

// template <typename Sigs>
// using value_types_of = ...;

// ============================================================
// TODO [必做]: implement error_types_of<Sigs>
// Extract all set_error_t<...> signatures from completion_signatures.
// Input:  completion_signatures<set_value_t<int>, set_error_t<std::exception_ptr>, set_stopped_t>
// Output: type_list<set_error_t<std::exception_ptr>>
//
// Hint: same approach as value_types_of, but with is_set_error predicate.
// ============================================================

// template <typename Sigs>
// using error_types_of = ...;

// ============================================================
// TODO [必做]: implement sends_stopped<Sigs>
// Compile-time bool constant: does Sigs contain set_stopped_t?
//
// Hint: use contains_v on sigs_to_list_t<Sigs>.
// ============================================================

// template <typename Sigs>
// inline constexpr bool sends_stopped = ...;

// ============================================================
// TODO [必做]: implement simplified make_completion_signatures
// Given InputSigs and a ValueTransform meta-function, produce OutputSigs:
//   1. Extract value types from InputSigs
//   2. Apply ValueTransform to each value type
//   3. Keep original error types and stopped
//   4. Assemble into new completion_signatures
//
// ValueTransform is a template<typename> class with a nested ::type.
// For example, to transform set_value_t<int> -> set_value_t<std::string>,
// the ValueTransform for set_value_t<int> should produce set_value_t<std::string>.
//
// Hint: use transform<value_types_of<InputSigs>, ValueTransform>,
//       then concat with error_types_of<InputSigs>,
//       conditionally add set_stopped_t,
//       and convert to completion_signatures via list_to_sigs_t.
// ============================================================

// template <typename InputSigs, template <typename> class ValueTransform>
// using make_completion_signatures = ...;

// ============================================================
// TODO [必做]: simulate then(f) signature transformation
//
// Scenario: f : int -> std::string
// Input signatures:
//   completion_signatures<set_value_t<int>, set_error_t<std::exception_ptr>, set_stopped_t>
// Expected output:
//   completion_signatures<set_value_t<std::string>, set_error_t<std::exception_ptr>, set_stopped_t>
//
// Define a ValueTransform meta-function that transforms set_value_t<int>
// to set_value_t<std::string> (generalizing: transforms set_value_t<Args...>
// by applying f's return type).
//
// Then use make_completion_signatures and static_assert the result.
// ============================================================

// Example ValueTransform for a function int -> std::string:
//
// template <typename Sig>
// struct then_value_transform;
//
// template <typename... Args>
// struct then_value_transform<set_value_t<Args...>> {
//     // For simplicity, assume f takes Args... and returns std::string
//     using type = set_value_t<std::string>;
// };

// ============================================================
// TODO [进阶]: handle f returning void
// If f : int -> void, the output value completion should be set_value_t<>
// (no arguments), not set_value_t<void>.
// ============================================================

// ============================================================
// TODO [进阶]: handle multiple value completions
// If input has set_value_t<int> and set_value_t<float>, and f is auto->string,
// both become set_value_t<std::string>. Use unique to deduplicate.
// ============================================================

// ============================================================
// static_assert verification
// Uncomment and fill in after implementing the above.
// ============================================================

// using input_sigs = completion_signatures<
//     set_value_t<int>,
//     set_error_t<std::exception_ptr>,
//     set_stopped_t
// >;

// -- value_types_of --
// static_assert(std::is_same_v<
//     value_types_of<input_sigs>,
//     type_list<set_value_t<int>>
// >);

// -- error_types_of --
// static_assert(std::is_same_v<
//     error_types_of<input_sigs>,
//     type_list<set_error_t<std::exception_ptr>>
// >);

// -- sends_stopped --
// static_assert(sends_stopped<input_sigs>);

// -- make_completion_signatures with then(f) where f: int -> string --
// using output_sigs = make_completion_signatures<input_sigs, then_value_transform>;
// static_assert(std::is_same_v<
//     output_sigs,
//     completion_signatures<set_value_t<std::string>, set_error_t<std::exception_ptr>, set_stopped_t>
// >);

int main() {
    // All verification is compile-time via static_assert.
    // If this file compiles, all checks pass.
    return 0;
}
