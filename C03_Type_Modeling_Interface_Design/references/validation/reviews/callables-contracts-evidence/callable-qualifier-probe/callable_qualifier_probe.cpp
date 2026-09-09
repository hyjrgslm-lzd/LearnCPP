#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>

#include "check.hpp"

namespace {

struct LvalueOnly {
    int operator()() & {
        return 42;
    }
};

struct ConstNoexcept {
    int operator()() const noexcept {
        return 7;
    }
};

} // namespace

int main() {
    std::move_only_function<int() &> only_lvalue{LvalueOnly{}};
    static_assert(std::is_invocable_v<decltype(only_lvalue)&>);
    static_assert(!std::is_invocable_v<decltype(only_lvalue)&&>);
    check(only_lvalue() == 42, "ref-qualified move_only_function calls only on lvalues");

    std::move_only_function<int() const noexcept> const_noexcept{ConstNoexcept{}};
    static_assert(std::is_invocable_v<decltype(const_noexcept) const&>);
    static_assert(std::is_nothrow_invocable_v<decltype(const_noexcept) const&>);
    const auto& as_const = const_noexcept;
    check(as_const() == 7, "const noexcept signature preserves const and noexcept invocation");

    int borrowed = 3;
    auto by_reference = std::ref(borrowed);
    auto by_value = borrowed;
    borrowed = 9;
    check(by_reference.get() == 9, "reference_wrapper observes the borrowed object");
    check(by_value == 3, "ordinary value capture keeps a separate copy");

    auto owner = std::make_shared<int>(11);
    std::function<int()> safe_owner_capture = [owner] { return *owner; };
    check(safe_owner_capture() == 11, "owning capture keeps pointee alive");
}
