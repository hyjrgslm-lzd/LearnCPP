// ============================================================
// Exercise E-2: tag_invoke infrastructure
// ============================================================
// Goal: Build a minimal tag_invoke dispatch system, define
//       connect and get_scheduler CPOs through it, and verify
//       the full dispatch chain.
// ============================================================

#include <iostream>
#include <string>
#include <type_traits>
#include <concepts>
#include <utility>

// ============================================================
// tag_invoke infrastructure
// ============================================================
namespace my_framework {

    // Poison pill -- ensures compile error when no user
    // customization is found, rather than silent fallthrough.
    namespace _ti_detail {
        void tag_invoke() = delete; // poison pill
    }
    using _ti_detail::tag_invoke;

    // TODO [必做]: Declare tag_invoke as the single ADL entry point.
    //   The poison pill above is already in place.  Users provide
    //   customizations via:
    //     friend auto tag_invoke(SomeTag, MyType, Args...) { ... }
    //   and ADL finds it because MyType is an argument.

    // --------------------------------------------------------
    // connect_t -- tag type for the connect protocol
    // --------------------------------------------------------
    struct connect_t {
        template <typename Sender, typename Receiver>
        auto operator()(Sender&& s, Receiver&& r) const {
            // TODO [必做]: dispatch through tag_invoke
            //   return tag_invoke(connect_t{}, std::forward<Sender>(s),
            //                     std::forward<Receiver>(r));
            //
            // Placeholder (remove after implementing):
            (void)s; (void)r;
            return 0;
        }
    };
    inline constexpr connect_t connect{};

    // --------------------------------------------------------
    // get_scheduler_t -- tag type for the get_scheduler query
    // --------------------------------------------------------
    struct get_scheduler_t {
        template <typename Env>
        auto operator()(Env const& env) const {
            // TODO [必做]: similar dispatch through tag_invoke
            //   return tag_invoke(get_scheduler_t{}, env);
            //
            // Placeholder (remove after implementing):
            (void)env;
            return 0;
        }
    };
    inline constexpr get_scheduler_t get_scheduler{};

} // namespace my_framework

// ============================================================
// User types
// ============================================================

// A trivial "operation state" returned by connect
struct my_op_state {
    int value;
    void start() { std::cout << "  my_op_state::start() -- value=" << value << "\n"; }
};

// A trivial receiver
struct my_receiver {
    std::string label{"default"};
};

// ---- my_sender ----
struct my_sender {
    int value;

    // TODO [必做]: provide connect customization via friend tag_invoke
    //   friend auto tag_invoke(my_framework::connect_t,
    //                          my_sender s, my_receiver r) {
    //       std::cout << "  tag_invoke(connect_t, my_sender{"
    //                 << s.value << "}, receiver=\""
    //                 << r.label << "\")\n";
    //       return my_op_state{s.value};
    //   }
};

// A trivial scheduler stand-in
struct my_scheduler {
    std::string name;
};

// ---- my_env ----
struct my_env {
    my_scheduler sched;

    // TODO [必做]: provide get_scheduler customization via friend tag_invoke
    //   friend auto tag_invoke(my_framework::get_scheduler_t,
    //                          my_env const& self) {
    //       std::cout << "  tag_invoke(get_scheduler_t, my_env)\n";
    //       return self.sched;
    //   }
};

// ============================================================
// main
// ============================================================
int main() {
    std::cout << "===== connect dispatch =====\n";
    my_sender s{42};
    my_receiver r{"test-receiver"};

    // TODO [必做]: call my_framework::connect(s, r) and verify dispatch
    // auto op = my_framework::connect(s, r);
    // op.start();

    std::cout << "\n===== get_scheduler dispatch =====\n";
    my_env env{my_scheduler{"thread_pool"}};

    // TODO [必做]: call my_framework::get_scheduler(env) and verify dispatch
    // auto sched = my_framework::get_scheduler(env);
    // std::cout << "  scheduler name: " << sched.name << "\n";

    // TODO [进阶]: add a start_t tag + CPO, wire up the full
    //   connect -> operation_state -> start chain through tag_invoke.

    // TODO [进阶]: add concept constraint tag_invocable<Tag, Args...>
    //   and use it in the CPO operator().

    std::cout << "\nE2: tag_invoke exercise -- implement the TODOs above.\n";
    return 0;
}
