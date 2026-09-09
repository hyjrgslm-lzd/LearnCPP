#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <exception>
#include <utility>

namespace ex = stdexec;

// ============ Forward declaration ============
template <typename Receiver>
struct my_operation_state;

// ============ Minimal sender ============
struct single_value_sender {
    using sender_concept = ex::sender_t;

    int value_;

    // TODO [必做]: Define completion_signatures.
    //   This sender can produce:
    //     - set_value_t(int)              on success
    //     - set_error_t(std::exception_ptr) on failure
    //   Declare them so sync_wait and other combinators can work.
    using completion_signatures = ex::completion_signatures<
        ex::set_value_t(int),
        ex::set_error_t(std::exception_ptr)
    >;

    // TODO [必做]: Implement connect.
    //   connect takes a receiver and returns an operation_state.
    template <typename Receiver>
    friend auto tag_invoke(ex::connect_t, single_value_sender self, Receiver rcvr) {
        // TODO [必做]: Return my_operation_state<Receiver> holding
        //   the value and the receiver.
        return my_operation_state<Receiver>{self.value_, std::move(rcvr)};
    }
};

// ============ Minimal operation_state ============
template <typename Receiver>
struct my_operation_state {
    using operation_state_concept = ex::operation_state_t;

    // TODO [必做]: Add members to hold the value and the receiver.
    int value_;
    Receiver rcvr_;

    // TODO [必做]: Implement start().
    //   start() is where actual execution happens.
    //   Call ex::set_value(std::move(rcvr_), value_) to deliver the result.
    friend void tag_invoke(ex::start_t, my_operation_state& self) noexcept {
        try {
            ex::set_value(std::move(self.rcvr_), self.value_);
        } catch (...) {
            ex::set_error(std::move(self.rcvr_), std::current_exception());
        }
    }
};

// ============ Logging receiver (from Exercise 11) ============
// Minimal version for testing. If you have completed Exercise 11,
// you can reuse your logging_receiver from there.
struct logging_receiver {
    using receiver_concept = ex::receiver_t;

    std::string name;

    friend void tag_invoke(ex::set_value_t, logging_receiver&& self, auto&&... values) {
        std::cout << "[" << self.name << "] set_value: ";
        ((std::cout << values << " "), ...);
        std::cout << "\n";
    }

    friend void tag_invoke(ex::set_error_t, logging_receiver&& self, auto&& error) {
        std::cout << "[" << self.name << "] set_error: ";
        try {
            if constexpr (std::is_same_v<std::decay_t<decltype(error)>, std::exception_ptr>) {
                if (error) std::rethrow_exception(error);
            } else {
                std::cout << error;
            }
        } catch (const std::exception& e) {
            std::cout << e.what();
        }
        std::cout << "\n";
    }

    friend void tag_invoke(ex::set_stopped_t, logging_receiver&& self) noexcept {
        std::cout << "[" << self.name << "] set_stopped\n";
    }

    friend auto tag_invoke(ex::get_env_t, const logging_receiver& self) {
        return ex::empty_env{};
    }
};

int main() {
    std::cout << "===== Exercise 12: Minimal Sender & operation_state =====\n\n";

    // ============ Test 1: Manual connect + start with logging_receiver ============
    {
        std::cout << "--- Test 1: Manual connect + start ---\n";

        single_value_sender sndr{42};

        // TODO [必做]: Connect the sender with logging_receiver, then start.
        auto op = ex::connect(std::move(sndr), logging_receiver{"manual_test"});
        ex::start(op);

        std::cout << "\n";
    }

    // ============ Test 2: Same sender, different receiver ============
    {
        std::cout << "--- Test 2: Same sender blueprint, new receiver ---\n";

        // Sender is a blueprint - each connect creates a NEW operation_state.
        single_value_sender sndr{100};

        auto op1 = ex::connect(sndr, logging_receiver{"receiver_A"});
        auto op2 = ex::connect(sndr, logging_receiver{"receiver_B"});

        ex::start(op1);
        ex::start(op2);

        std::cout << "\n";
    }

    // ============ Test 3: Verify with sync_wait ============
    {
        std::cout << "--- Test 3: sync_wait verification ---\n";

        // TODO [必做]: Use sync_wait to consume the sender.
        //   If completion_signatures are correct, this should compile and work.
        //   If sync_wait fails to compile, check your completion_signatures!
        single_value_sender sndr{77};
        auto result = ex::sync_wait(std::move(sndr));
        if (result) {
            auto [val] = result.value();
            std::cout << "sync_wait got: " << val << "\n";
        }

        std::cout << "\n";
    }

    // ============ Test 4: Compose with then ============
    {
        std::cout << "--- Test 4: Compose with then ---\n";

        // If completion_signatures are correct, standard combinators work.
        auto sndr = single_value_sender{10}
            | ex::then([](int x) {
                std::cout << "then received: " << x << "\n";
                return x * 2;
            });

        auto result = ex::sync_wait(std::move(sndr));
        if (result) {
            auto [val] = result.value();
            std::cout << "Final result: " << val << "\n";
        }

        std::cout << "\n";
    }

    // TODO [进阶]: Make the sender configurable to send value, error, or stopped
    //   based on a mode flag. Update completion_signatures accordingly:
    //     using completion_signatures = ex::completion_signatures<
    //         ex::set_value_t(int),
    //         ex::set_error_t(std::exception_ptr),
    //         ex::set_stopped_t()
    //     >;

    // TODO [进阶]: Deliberately write WRONG completion_signatures
    //   (e.g. declare set_value_t(int) but actually send a std::string)
    //   and observe the compiler error. This demonstrates why signatures matter.

    std::cout << "===== Done =====\n";
    return 0;
}
