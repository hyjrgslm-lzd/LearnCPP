#include <type_traits>
#include <exception>
#include <string>
#include <concepts>

// ============================================================
// Simplified completion_signatures infrastructure (from F-2)
// ============================================================

template <typename... Args>
struct set_value_t {};

template <typename Err>
struct set_error_t {};

struct set_stopped_t {};

template <typename... Sigs>
struct completion_signatures {};

// ============================================================
// TODO [必做]: define my_sender concept
//
// A type S satisfies my_sender if:
//   - S has a nested type alias `sender_concept`
//   - S has a nested type alias `completion_signatures`
//     (or provides get_completion_signatures)
//
// Hint:
//   template <typename S>
//   concept my_sender = requires {
//       typename S::sender_concept;
//       typename S::completion_signatures;
//   };
// ============================================================

// template <typename S>
// concept my_sender = ...;

// ============================================================
// TODO [必做]: define my_receiver concept
//
// A type R satisfies my_receiver if:
//   - R has a nested type alias `receiver_concept`
//   - R supports set_stopped() (no arguments)
//
// Note: value and error parameter checking is deferred to my_receiver_of.
//
// Hint:
//   template <typename R>
//   concept my_receiver = requires(std::remove_cvref_t<R>& r) {
//       typename std::remove_cvref_t<R>::receiver_concept;
//       r.set_stopped();
//   };
// ============================================================

// template <typename R>
// concept my_receiver = ...;

// ============================================================
// TODO [必做]: define my_receiver_of<R, Sigs> concept
//
// Check that receiver R can handle every completion in Sigs.
// For each set_value_t<Args...> in Sigs, R must support set_value(Args...).
// For each set_error_t<Err> in Sigs, R must support set_error(Err).
// If set_stopped_t is present, R must support set_stopped().
//
// Simplification: start with a single set_value_t<T> case, then
// generalize to multiple value completions if desired.
//
// Hint: you can use helper concepts or constexpr functions to check
// each signature in the pack.
//
// template <typename R, typename Sigs>
// concept my_receiver_of = my_receiver<R> && /* check each Sig in Sigs */;
// ============================================================

// Helper: check one signature against a receiver
// template <typename R, typename Sig>
// concept can_handle_sig = ...;

// template <typename R, typename Sigs>
// concept my_receiver_of = ...;

// ============================================================
// TODO [进阶]: define my_sender_to<S, R> concept
//
// template <typename S, typename R>
// concept my_sender_to =
//     my_sender<S> &&
//     my_receiver<R> &&
//     my_receiver_of<R, typename S::completion_signatures>;
// ============================================================

// template <typename S, typename R>
// concept my_sender_to = ...;

// ============================================================
// Test types
// ============================================================

// -- valid_sender: satisfies my_sender --
struct valid_sender {
    using sender_concept = void;  // simplified tag
    using completion_signatures = ::completion_signatures<
        set_value_t<int>,
        set_error_t<std::exception_ptr>,
        set_stopped_t
    >;
};

// -- invalid_sender: does NOT satisfy my_sender (missing completion_signatures) --
struct invalid_sender {
    using sender_concept = void;
    // no completion_signatures
};

// -- another_invalid: not a sender at all --
struct not_a_sender {
    int x;
};

// -- valid_receiver: handles int value, exception_ptr error, and stopped --
struct valid_receiver {
    using receiver_concept = void;  // simplified tag

    void set_value(int) {}
    void set_error(std::exception_ptr) {}
    void set_stopped() {}
};

// -- invalid_receiver: missing receiver_concept --
struct invalid_receiver {
    void set_value(int) {}
    void set_error(std::exception_ptr) {}
    void set_stopped() {}
};

// -- partial_receiver: has receiver_concept but only accepts string, not int --
struct partial_receiver {
    using receiver_concept = void;

    void set_value(std::string) {}
    void set_error(std::exception_ptr) {}
    void set_stopped() {}
};

// ============================================================
// static_assert tests
// Uncomment after implementing the concepts above.
// ============================================================

// -- my_sender tests --
// static_assert(my_sender<valid_sender>);
// static_assert(!my_sender<invalid_sender>);
// static_assert(!my_sender<not_a_sender>);
// static_assert(!my_sender<int>);

// -- my_receiver tests --
// static_assert(my_receiver<valid_receiver>);
// static_assert(!my_receiver<invalid_receiver>);
// static_assert(!my_receiver<int>);

// -- my_receiver_of tests --
// using test_sigs = completion_signatures<
//     set_value_t<int>,
//     set_error_t<std::exception_ptr>,
//     set_stopped_t
// >;
// static_assert(my_receiver_of<valid_receiver, test_sigs>);
// static_assert(!my_receiver_of<partial_receiver, test_sigs>);  // can't handle set_value(int)

// -- my_sender_to tests (进阶) --
// static_assert(my_sender_to<valid_sender, valid_receiver>);
// static_assert(!my_sender_to<valid_sender, partial_receiver>);
// static_assert(!my_sender_to<invalid_sender, valid_receiver>);

int main() {
    // All verification is compile-time via static_assert.
    // If this file compiles, all checks pass.
    return 0;
}
