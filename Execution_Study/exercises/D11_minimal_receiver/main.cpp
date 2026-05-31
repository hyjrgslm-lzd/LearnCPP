#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <exception>

namespace ex = stdexec;

// ============ Logging Receiver ============
struct logging_receiver {
    using receiver_concept = ex::receiver_t;

    std::string name; // identifier for log messages

    // ---- set_value: value completion channel ----
    friend void tag_invoke(ex::set_value_t, logging_receiver&& self, auto&&... values) {
        // TODO [必做]: Print the receiver name and the received value(s).
        //   Example output: "[my_receiver] set_value: 42"
        //   Hint: use a fold expression to print variadic values:
        //     (std::cout << ... << values);
        std::cout << "[" << self.name << "] set_value: ";
        ((std::cout << values << " "), ...);
        std::cout << "\n";
    }

    // ---- set_error: error completion channel ----
    friend void tag_invoke(ex::set_error_t, logging_receiver&& self, auto&& error) {
        // TODO [必做]: Print the receiver name and error information.
        //   If error is std::exception_ptr, rethrow to extract the message.
        //   Example output: "[my_receiver] set_error: <message>"
        std::cout << "[" << self.name << "] set_error: ";
        try {
            if constexpr (std::is_same_v<std::decay_t<decltype(error)>, std::exception_ptr>) {
                if (error) std::rethrow_exception(error);
            } else {
                std::cout << error;
            }
        } catch (const std::exception& e) {
            std::cout << e.what();
        } catch (...) {
            std::cout << "(unknown error)";
        }
        std::cout << "\n";
    }

    // ---- set_stopped: stopped completion channel ----
    friend void tag_invoke(ex::set_stopped_t, logging_receiver&& self) noexcept {
        // TODO [必做]: Print that this receiver received a stopped signal.
        //   Example output: "[my_receiver] set_stopped"
        std::cout << "[" << self.name << "] set_stopped\n";
    }

    // ---- get_env: environment entry point ----
    friend auto tag_invoke(ex::get_env_t, const logging_receiver& self) {
        // TODO [必做]: Return a minimal environment.
        //   ex::empty_env{} is the simplest valid choice.
        //   For a more useful environment, consider returning one with
        //   a never_stop_token.
        return ex::empty_env{};
    }
};

int main() {
    std::cout << "===== Exercise 11: Minimal Receiver =====\n\n";

    // ============ Test 1: Value path (just(42)) ============
    {
        std::cout << "--- Test 1: Value path ---\n";

        auto sndr = ex::just(42);

        // TODO [必做]: Connect the sender with a logging_receiver, then start it.
        //   auto op = ex::connect(std::move(sndr), logging_receiver{"value_test"});
        //   ex::start(op);
        auto op = ex::connect(std::move(sndr), logging_receiver{"value_test"});
        ex::start(op);

        std::cout << "\n";
    }

    // ============ Test 2: Error path ============
    {
        std::cout << "--- Test 2: Error path ---\n";

        // TODO [必做]: Create a sender that completes via the error channel.
        //   Use ex::just_error(std::make_exception_ptr(
        //       std::runtime_error("test error")))
        //   Connect with logging_receiver and start.
        auto sndr = ex::just_error(
            std::make_exception_ptr(std::runtime_error("test error"))
        );
        auto op = ex::connect(std::move(sndr), logging_receiver{"error_test"});
        ex::start(op);

        std::cout << "\n";
    }

    // ============ Test 3: Stopped path ============
    {
        std::cout << "--- Test 3: Stopped path ---\n";

        // TODO [必做]: Create a sender that completes via the stopped channel.
        //   Use ex::just_stopped()
        //   Connect with logging_receiver and start.
        auto sndr = ex::just_stopped();
        auto op = ex::connect(std::move(sndr), logging_receiver{"stopped_test"});
        ex::start(op);

        std::cout << "\n";
    }

    // ============ Test 4: Value with multiple arguments ============
    {
        std::cout << "--- Test 4: Multiple values ---\n";

        auto sndr = ex::just(10, std::string("hello"), 3.14);
        auto op = ex::connect(std::move(sndr), logging_receiver{"multi_test"});
        ex::start(op);

        std::cout << "\n";
    }

    // TODO [进阶]: Enhance the receiver to log events into a struct
    //   instead of just printing. For example:
    //   struct EventLog {
    //       std::string channel; // "value", "error", "stopped"
    //       std::string detail;
    //   };
    //   Store events in a vector and print the log at the end.

    // TODO [进阶]: Return a more interesting environment from get_env(),
    //   e.g. one that provides a never_stop_token:
    //   return ex::make_env(ex::with(ex::get_stop_token, ex::never_stop_token{}));

    std::cout << "===== Done =====\n";
    return 0;
}
