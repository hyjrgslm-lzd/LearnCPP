// ============================================================
// Exercise D13: Minimal sender adaptor -- tap
// ============================================================
// Goal: Wrap an inner sender, intercept value completion to
//       execute a side-effect, then forward values downstream.
// ============================================================

#include <stdexec/execution.hpp>
#include <iostream>
#include <tuple>
#include <utility>

namespace ex = stdexec;

// ============================================================
// tap_receiver<DownstreamReceiver, F>
// ============================================================
// Wraps the downstream receiver. On set_value, calls f first,
// then forwards the values unchanged.
// ============================================================
template <typename DownstreamReceiver, typename F>
struct tap_receiver {
    using receiver_concept = ex::receiver_t;

    DownstreamReceiver downstream_;
    F f_;

    // TODO [必做]: set_value -- call f_(values...) as side-effect,
    //              then forward ex::set_value(std::move(downstream_), values...)
    // friend void tag_invoke(ex::set_value_t, tap_receiver&& self, auto&&... values) noexcept { ... }

    // TODO [必做]: set_error -- forward to downstream without modification
    // friend void tag_invoke(ex::set_error_t, tap_receiver&& self, auto&& err) noexcept { ... }

    // TODO [必做]: set_stopped -- forward to downstream
    // friend void tag_invoke(ex::set_stopped_t, tap_receiver&& self) noexcept { ... }

    // TODO [必做]: get_env -- forward to downstream
    // friend auto tag_invoke(ex::get_env_t, const tap_receiver& self) noexcept { ... }
};

// ============================================================
// tap_operation_state<InnerSender, DownstreamReceiver, F>
// ============================================================
// Holds the operation state produced by connecting InnerSender
// with tap_receiver<DownstreamReceiver, F>.
// ============================================================
// TODO [必做]: define tap_operation_state
//   - It should store the inner operation state produced by
//     ex::connect(inner_sender, tap_receiver{downstream, f})
//   - It needs: using operation_state_concept = ex::operation_state_t;
//   - It needs: friend void tag_invoke(ex::start_t, tap_operation_state& self) noexcept
//               that calls ex::start on the inner operation state.
//
// template <typename InnerSender, typename DownstreamReceiver, typename F>
// struct tap_operation_state {
//     using operation_state_concept = ex::operation_state_t;
//     using inner_op_t = ex::connect_result_t<InnerSender, tap_receiver<DownstreamReceiver, F>>;
//     inner_op_t inner_op_;
//
//     tap_operation_state(InnerSender inner, DownstreamReceiver downstream, F f)
//         : inner_op_(ex::connect(std::move(inner),
//                                 tap_receiver<DownstreamReceiver, F>{
//                                     std::move(downstream), std::move(f)})) {}
//
//     friend void tag_invoke(ex::start_t, tap_operation_state& self) noexcept {
//         ex::start(self.inner_op_);
//     }
// };

// ============================================================
// tap_sender<InnerSender, F>
// ============================================================
template <typename InnerSender, typename F>
struct tap_sender {
    using sender_concept = ex::sender_t;

    InnerSender inner_;
    F f_;

    // TODO [必做]: completion_signatures -- should be the same as the inner sender.
    //   Use ex::completion_signatures_of_t<InnerSender, ex::empty_env> or
    //   forward the inner sender's signatures directly.
    // using completion_signatures = ex::completion_signatures_of_t<InnerSender, ex::empty_env>;

    // TODO [必做]: connect -- produce a tap_operation_state
    // template <typename Receiver>
    // friend auto tag_invoke(ex::connect_t, tap_sender&& self, Receiver rcvr) {
    //     return tap_operation_state<InnerSender, Receiver, F>{
    //         std::move(self.inner_), std::move(rcvr), std::move(self.f_)};
    // }
};

// ============================================================
// Factory function
// ============================================================
template <typename Sender, typename F>
auto tap(Sender sndr, F f) {
    // TODO [必做]: return a tap_sender wrapping sndr and f
    return tap_sender<Sender, F>{std::move(sndr), std::move(f)};
}

// ============================================================
// TODO [进阶]: map adaptor -- like tap but f transforms the value
// template <typename Sender, typename F>
// auto map(Sender sndr, F f) { ... }
// ============================================================

// ============================================================
// TODO [进阶]: pipe operator support
// template <typename F>
// auto tap(F f) { return [f](auto sndr) { return tap(std::move(sndr), f); }; }
// template <typename Sender, typename Adaptor>
// auto operator|(Sender sndr, Adaptor adaptor) { return adaptor(std::move(sndr)); }
// ============================================================

int main() {
    // ---- Verification ----
    // After completing all TODOs, uncomment the following:

    // auto result = ex::sync_wait(
    //     tap(ex::just(42), [](int x) {
    //         std::cout << "tap: " << x << "\n";
    //     })
    // );
    //
    // // result is std::optional<std::tuple<int>>
    // if (result) {
    //     auto [value] = *result;
    //     std::cout << "result: " << value << "\n";
    //     // Expected output:
    //     //   tap: 42
    //     //   result: 42
    // }

    std::cout << "D13: tap adaptor exercise -- implement the TODOs above.\n";
    return 0;
}
