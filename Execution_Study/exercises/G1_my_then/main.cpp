#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <tuple>

namespace ex = stdexec;

// ============================================================
// my_then_receiver
// ============================================================

template <typename DownstreamReceiver, typename F>
struct my_then_receiver {
    using receiver_concept = ex::receiver_t;

    DownstreamReceiver downstream_;
    F f_;

    // TODO [必做]: set_value - apply f_ to the incoming values, forward result to downstream
    //
    // friend void tag_invoke(ex::set_value_t, my_then_receiver&& self, auto&&... values) {
    //     // Call f_(values...) and send the result downstream.
    //     // If f_ returns void, send set_value() with no arguments.
    //     auto result = self.f_(std::forward<decltype(values)>(values)...);
    //     ex::set_value(std::move(self.downstream_), std::move(result));
    // }

    // TODO [必做]: set_error - transparent forward to downstream
    //
    // friend void tag_invoke(ex::set_error_t, my_then_receiver&& self, auto&& err) {
    //     ex::set_error(std::move(self.downstream_), std::forward<decltype(err)>(err));
    // }

    // TODO [必做]: set_stopped - transparent forward to downstream
    //
    // friend void tag_invoke(ex::set_stopped_t, my_then_receiver&& self) noexcept {
    //     ex::set_stopped(std::move(self.downstream_));
    // }

    // TODO [必做]: get_env - forward to downstream receiver's environment
    //
    // friend auto tag_invoke(ex::get_env_t, const my_then_receiver& self) noexcept {
    //     return ex::get_env(self.downstream_);
    // }
};

// ============================================================
// my_then_operation_state
// ============================================================

template <typename InnerSender, typename DownstreamReceiver, typename F>
struct my_then_operation_state {
    using operation_state_concept = ex::operation_state_t;

    // TODO [必做]: inner_op_ member
    // The type is the result of connecting InnerSender with my_then_receiver<DownstreamReceiver, F>.
    // Use ex::connect_result_t<InnerSender, my_then_receiver<DownstreamReceiver, F>> for the type.
    //
    // using inner_receiver_t = my_then_receiver<DownstreamReceiver, F>;
    // using inner_op_t = ex::connect_result_t<InnerSender, inner_receiver_t>;
    // inner_op_t inner_op_;

    // Constructor: connect inner sender with my_then_receiver
    // my_then_operation_state(InnerSender&& sender, DownstreamReceiver&& receiver, F&& f)
    //     : inner_op_(ex::connect(
    //           std::move(sender),
    //           inner_receiver_t{std::move(receiver), std::move(f)}
    //       ))
    // {}

    // TODO [必做]: start() - delegates to inner_op_
    //
    // friend void tag_invoke(ex::start_t, my_then_operation_state& self) noexcept {
    //     ex::start(self.inner_op_);
    // }
};

// ============================================================
// my_then_sender
// ============================================================

template <typename InnerSender, typename F>
struct my_then_sender {
    using sender_concept = ex::sender_t;

    InnerSender inner_;
    F f_;

    // TODO [必做]: completion_signatures
    // Derive from InnerSender's value types + F's return type.
    //
    // For simplification, assume InnerSender sends a single set_value(T):
    //   - If F(T) returns R, output set_value(R)
    //   - Error and stopped signatures pass through from InnerSender
    //
    // For a minimal working version, you can hardcode:
    //   using completion_signatures = ex::completion_signatures_of_t<InnerSender, ex::empty_env>;
    // Then adjust the value types based on F's return type.
    //
    // A more precise approach uses ex::make_completion_signatures or
    // manually constructs the signatures.

    // TODO [必做]: connect implementation
    //
    // template <typename Receiver>
    // friend auto tag_invoke(ex::connect_t, my_then_sender&& self, Receiver&& receiver) {
    //     return my_then_operation_state<InnerSender, std::remove_cvref_t<Receiver>, F>{
    //         std::move(self.inner_),
    //         std::forward<Receiver>(receiver),
    //         std::move(self.f_)
    //     };
    // }
};

// ============================================================
// Factory function
// ============================================================

template <typename Sender, typename F>
auto my_then(Sender s, F f) {
    return my_then_sender<Sender, F>{std::move(s), std::move(f)};
}

// ============================================================
// Tests
// ============================================================

int main() {
    // Test 1: basic value transform (int -> int)
    // auto r1 = ex::sync_wait(my_then(ex::just(42), [](int x){ return x + 1; }));
    // std::cout << "Result: " << std::get<0>(*r1) << " (expect 43)\n";

    // Test 2: type-changing transform (int -> string)
    // auto r2 = ex::sync_wait(my_then(ex::just(42), [](int x){ return std::to_string(x); }));
    // std::cout << "Result: " << std::get<0>(*r2) << " (expect '42')\n";

    // Test 3: chained my_then
    // auto r3 = ex::sync_wait(
    //     my_then(
    //         my_then(ex::just(10), [](int x){ return x * 2; }),
    //         [](int x){ return x + 3; }
    //     )
    // );
    // std::cout << "Result: " << std::get<0>(*r3) << " (expect 23)\n";

    std::cout << "All my_then tests passed.\n";
    return 0;
}
