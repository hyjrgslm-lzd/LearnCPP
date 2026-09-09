// ============================================================
// Exercise E-3: Evolution compare & Member-first Dispatch
// ============================================================
// Goal: Refactor E-2's tag_invoke sender to use member-function
//       dispatch, then compare all four customization mechanisms.
// ============================================================

#include <iostream>
#include <string>
#include <type_traits>
#include <concepts>
#include <utility>

// ============================================================
// Minimal tag_invoke infrastructure (carried over from E-2)
// ============================================================
namespace fw {
    namespace _ti_detail { void tag_invoke() = delete; }
    using _ti_detail::tag_invoke;

    struct connect_t {};
    inline constexpr connect_t connect_tag{};
}

// ============================================================
// A trivial operation state and receiver (shared by both versions)
// ============================================================
struct trivial_op_state {
    int value;
    void start() { std::cout << "  trivial_op_state::start() value=" << value << "\n"; }
};

struct trivial_receiver {
    std::string label{"rx"};
};

// ============================================================
// Member-first dispatch version
// ============================================================
struct my_sender_v2 {
    int value;

    // TODO [必做]: define connect() as a member function
    //   auto connect(trivial_receiver r) const {
    //       std::cout << "  [member] my_sender_v2::connect(receiver=\""
    //                 << r.label << "\")\n";
    //       return trivial_op_state{value};
    //   }
};

// ============================================================
// tag_invoke version (for comparison / third-party fallback)
// ============================================================
struct third_party_sender {
    int value;

    // Cannot modify this type -- use tag_invoke to customize.
    // TODO [必做]: friend tag_invoke for connect
    //   friend auto tag_invoke(fw::connect_t, third_party_sender s, trivial_receiver r) {
    //       std::cout << "  [tag_invoke] third_party_sender::connect(receiver=\""
    //                 << r.label << "\")\n";
    //       return trivial_op_state{s.value};
    //   }
};

// ============================================================
// CPO with member-first + tag_invoke fallback
// ============================================================
struct connect_cpo_v2 {
    template <typename Sender, typename Receiver>
    auto operator()(Sender&& s, Receiver&& r) const {
        // TODO [必做]: Priority 1 -- try s.connect(r) (member function)
        //   if constexpr (requires { s.connect(r); }) {
        //       return s.connect(std::forward<Receiver>(r));
        //   }

        // TODO [必做]: Priority 2 -- fallback to tag_invoke
        //   else if constexpr (requires { tag_invoke(fw::connect_tag,
        //                                            std::forward<Sender>(s),
        //                                            std::forward<Receiver>(r)); }) {
        //       return tag_invoke(fw::connect_tag,
        //                         std::forward<Sender>(s),
        //                         std::forward<Receiver>(r));
        //   }

        //   else {
        //       static_assert(false, "No connect customization found");
        //   }

        // Placeholder -- remove once TODOs are implemented:
        (void)s; (void)r;
        return trivial_op_state{0};
    }
};

inline constexpr connect_cpo_v2 my_connect{};

// ============================================================
// main
// ============================================================
int main() {
    trivial_receiver rx{"test"};

    std::cout << "===== Member dispatch (my_sender_v2) =====\n";
    my_sender_v2 s1{10};
    // TODO [必做]: test member dispatch
    // auto op1 = my_connect(s1, rx);
    // op1.start();

    std::cout << "\n===== tag_invoke fallback (third_party_sender) =====\n";
    third_party_sender s2{20};
    // TODO [必做]: test tag_invoke fallback for third-party type
    // auto op2 = my_connect(s2, rx);
    // op2.start();

    // ============================================================
    // TODO [必做]: write comparison table as comments:
    // ============================================================
    //
    // | Mechanism      | Lines | Readability | Extensibility          | ADL Hijack Risk | Cross-cutting |
    // |----------------|-------|-------------|------------------------|-----------------|---------------|
    // | Raw ADL        |       |             |                        |                 |               |
    // | CPO            |       |             |                        |                 |               |
    // | tag_invoke     |       |             |                        |                 |               |
    // | Member-first   |       |             |                        |                 |               |
    //
    // Applicable scenarios:
    //   - Own type -> member function is simplest
    //   - Third-party type (no source access) -> tag_invoke is required
    //
    // Evolution summary:
    //   ADL (C++98) -> CPO/Niebloid (C++20) -> tag_invoke (P1895)
    //   -> member-first dispatch (P2855 / C++26)
    //

    // TODO [进阶]: implement a full set_value CPO with member-first
    //   dispatch and run a complete connect-start-complete chain.

    // TODO [进阶]: design a mixed-mode type that uses member connect()
    //   but tag_invoke for get_env().

    std::cout << "\nE3: evolution compare exercise -- implement the TODOs above.\n";
    return 0;
}
