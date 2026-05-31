#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <tuple>

namespace ex = stdexec;

// ============================================================
// my_then_sender / my_then_receiver / my_then_operation_state
// (Reuse or copy from G1_my_then — fill in the working versions)
// ============================================================

template <typename DownstreamReceiver, typename F>
struct my_then_receiver {
    using receiver_concept = ex::receiver_t;
    DownstreamReceiver downstream_;
    F f_;

    // TODO: copy working implementations from G1
    // set_value, set_error, set_stopped, get_env
};

template <typename InnerSender, typename DownstreamReceiver, typename F>
struct my_then_operation_state {
    using operation_state_concept = ex::operation_state_t;

    // TODO: copy working implementation from G1
};

template <typename InnerSender, typename F>
struct my_then_sender {
    using sender_concept = ex::sender_t;
    InnerSender inner_;
    F f_;

    // TODO: copy working implementation from G1
    // completion_signatures + connect
};

// Two-argument factory (from G1)
template <typename Sender, typename F>
auto my_then_2(Sender s, F f) {
    return my_then_sender<Sender, F>{std::move(s), std::move(f)};
}

// ============================================================
// TODO [进阶]: sender_adaptor_closure base for chaining
//
// A CRTP base class that provides operator| for both:
//   1. Sender | Closure  -> result of closure(sender)
//   2. Closure | Closure  -> composed closure
//
// template <typename Derived>
// struct sender_adaptor_closure {
//     // Sender | Closure
//     template <typename Sender>
//         requires (!std::derived_from<std::remove_cvref_t<Sender>, sender_adaptor_closure>)
//     friend auto operator|(Sender&& sender, Derived&& closure) {
//         return std::move(closure)(std::forward<Sender>(sender));
//     }
//
//     // Closure | Closure -> composed closure
//     template <typename OtherClosure>
//         requires std::derived_from<std::remove_cvref_t<OtherClosure>, sender_adaptor_closure<std::remove_cvref_t<OtherClosure>>>
//     friend auto operator|(OtherClosure&& lhs, Derived&& rhs) {
//         // return a new closure that applies lhs then rhs
//     }
// };
// ============================================================

// ============================================================
// TODO [必做]: my_then_closure<F> struct holding F
//
// This struct holds the function f and, when called with a sender,
// returns my_then_sender<Sender, F>.
//
// struct my_then_closure should inherit from sender_adaptor_closure
// (进阶) or be a standalone type (必做).
//
// template <typename F>
// struct my_then_closure {
//     F f_;
//
//     template <typename Sender>
//     auto operator()(Sender&& sender) && {
//         return my_then_2(std::forward<Sender>(sender), std::move(f_));
//     }
// };
// ============================================================

// ============================================================
// TODO [必做]: operator|(Sender, my_then_closure<F>)
//
// Free function or friend that enables: sender | my_then_closure{f}
//
// template <typename Sender, typename F>
// auto operator|(Sender&& sender, my_then_closure<F>&& closure) {
//     return std::move(closure)(std::forward<Sender>(sender));
// }
// ============================================================

// ============================================================
// TODO [必做]: my_then(F) single-argument factory returning closure
//
// template <typename F>
// auto my_then(F f) {
//     return my_then_closure<F>{std::move(f)};
// }
// ============================================================

// ============================================================
// Tests
// ============================================================

int main() {
    // Test 1: single pipe
    // auto r1 = ex::sync_wait(
    //     ex::just(1) | my_then([](int x){ return x * 2; })
    // );
    // std::cout << "Result: " << std::get<0>(*r1) << " (expect 2)\n";

    // Test 2: chained pipes
    // auto r2 = ex::sync_wait(
    //     ex::just(1)
    //     | my_then([](int x){ return x + 10; })
    //     | my_then([](int x){ return x * 3; })
    // );
    // std::cout << "Result: " << std::get<0>(*r2) << " (expect 33)\n";

    // Test 3: type-changing chain
    // auto r3 = ex::sync_wait(
    //     ex::just(42)
    //     | my_then([](int x){ return std::to_string(x); })
    //     | my_then([](std::string s){ return s + "!"; })
    // );
    // std::cout << "Result: " << std::get<0>(*r3) << " (expect '42!')\n";

    // Test 4 [进阶]: pre-composed closures
    // auto pipeline = my_then([](int x){ return x + 10; })
    //               | my_then([](int x){ return x * 3; });
    // auto r4 = ex::sync_wait(ex::just(1) | std::move(pipeline));
    // std::cout << "Result: " << std::get<0>(*r4) << " (expect 33)\n";

    std::cout << "All pipe syntax tests passed.\n";
    return 0;
}
