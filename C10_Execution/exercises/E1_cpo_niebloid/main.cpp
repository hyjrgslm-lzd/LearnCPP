// ============================================================
// Exercise E-1: CPO & Niebloid
// ============================================================
// Goal: Implement a minimal CPO, contrast it with raw ADL free
//       functions, and demonstrate ADL hijacking + its fix.
// ============================================================

#include <iostream>
#include <string>
#include <type_traits>
#include <concepts>

// We do not need stdexec for this exercise, but keeping the
// alias for consistency:
// namespace ex = stdexec;

// ============================================================
// v1: Raw ADL
// ============================================================
namespace lib_a {
    struct Cat {
        std::string name;
    };

    // TODO [必做]: ADL free function greet(Cat)
    //   e.g. inline void greet(Cat const& c) { std::cout << "lib_a greet: " << c.name << "\n"; }
}

namespace lib_b {
    struct Dog {
        std::string name;
    };

    // TODO [必做]: ADL free function greet(Dog)
    //   e.g. inline void greet(Dog const& d) { std::cout << "lib_b greet: " << d.name << "\n"; }
}

namespace third_party {
    // Widget inherits from Cat -- this creates an ADL hijacking scenario.
    struct Widget : lib_a::Cat {};

    // This "accidentally" defines greet too -- ADL may pick it up
    // when calling greet on a Widget, because Widget's associated
    // namespaces include both third_party and lib_a.
    inline void greet(Widget const& w) {
        std::cout << "third_party greet (HIJACKED): " << w.name << "\n";
    }
}

// ============================================================
// v2: CPO version
// ============================================================
namespace cpo_ns {

    // Internal ADL target -- a *different* name from the CPO itself
    // to avoid infinite recursion.  Types opt in by providing
    // either a .greet() member or a free greet_impl() found via ADL.
    // (see TODO items inside the CPO)

    struct greet_cpo_t {
        template <typename T>
        void operator()(T const& x) const {
            // TODO [必做]: Priority 1 -- try x.greet() (member function)
            //   if constexpr (requires { x.greet(); }) {
            //       x.greet();
            //   }

            // TODO [必做]: Priority 2 -- try greet_impl(x) via ADL
            //   else if constexpr (requires { greet_impl(x); }) {
            //       greet_impl(x);
            //   }

            // TODO [必做]: Priority 3 -- default fallback
            //   else {
            //       std::cout << "<unknown>\n";
            //   }

            // Placeholder -- remove once TODOs are implemented:
            (void)x;
            std::cout << "greet_cpo_t: not yet implemented\n";
        }
    };

    inline constexpr greet_cpo_t greet{};

} // namespace cpo_ns

// ============================================================
// Opt-in for CPO: lib_a::Cat via member function
// ============================================================
// TODO [必做]: Add a .greet() member to Cat, or provide a free
//   greet_impl(Cat) in namespace lib_a so the CPO can find it.
//   Example (member approach -- requires modifying Cat above):
//     void greet() const { std::cout << "CPO lib_a greet: " << name << "\n"; }
//   Example (ADL approach):
//     namespace lib_a {
//         inline void greet_impl(Cat const& c) { ... }
//     }

// ============================================================
// Opt-in for CPO: lib_b::Dog via ADL greet_impl
// ============================================================
// TODO [必做]: Provide greet_impl(Dog) in namespace lib_b.

// ============================================================
// main
// ============================================================
int main() {
    lib_a::Cat cat{"Whiskers"};
    lib_b::Dog dog{"Rex"};
    third_party::Widget widget{{"Gadget"}};

    std::cout << "===== v1: Raw ADL =====\n";
    // TODO [必做]: demonstrate ADL calls
    //   greet(cat);    // OK -- finds lib_a::greet
    //   greet(dog);    // OK -- finds lib_b::greet
    //   greet(widget); // PROBLEM -- may find third_party::greet (hijack!)

    std::cout << "\n===== v2: CPO =====\n";
    // TODO [必做]: demonstrate CPO prevents hijacking
    //   cpo_ns::greet(cat);
    //   cpo_ns::greet(dog);
    //   cpo_ns::greet(widget);  // Should use lib_a path, NOT third_party

    // TODO [进阶]: implement a name_of CPO that returns std::string_view
    //   with multi-layer fallback (member .name() -> ADL name_of_impl -> "<unnamed>").

    // TODO [进阶]: add concept constraints so that types without any
    //   opt-in get a clear static_assert instead of the default fallback.

    std::cout << "\nE1: CPO & Niebloid exercise -- implement the TODOs above.\n";
    return 0;
}
