#include <check.hpp>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {

struct Widget {
    int value = 3;

    int add(int delta) const { return value + delta; }
};

struct IncrementTarget {
    int value = 0;
    void increment() { ++value; }
};

struct MutableCall {
    int* calls;
    int operator()() {
        ++*calls;
        return *calls;
    }
};

struct LvalueOnly {
    int operator()() & { return 42; }
};

struct ConstNoexcept {
    int operator()() const noexcept { return 7; }
};

void check_invoke_forms()
{
    Widget widget{7};
    check(std::invoke(&Widget::add, widget, 5) == 12, "std::invoke calls member function");
    check(std::invoke(&Widget::value, widget) == 7, "std::invoke reads member data");
    auto twice = [](int value) { return value * 2; };
    check(std::invoke(twice, 6) == 12, "std::invoke calls function object");
}

void check_reference_wrapper_borrows()
{
    IncrementTarget target{};
    std::reference_wrapper<IncrementTarget> borrowed = target;
    borrowed.get().increment();
    check(target.value == 1, "reference_wrapper modifies borrowed object");
}

void check_function_contracts()
{
    std::function<int()> empty;
    bool threw = false;
    try {
        (void) empty();
    } catch (const std::bad_function_call&) {
        threw = true;
    }
    check(threw, "empty std::function throws bad_function_call");

    int calls = 0;
    const std::function<int()> callback = MutableCall{&calls};
    check(callback() == 1, "const std::function can call mutable target");
    check(calls == 1, "target mutation is visible through const std::function");
}

void check_move_only_function_qualifiers()
{
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
    std::move_only_function<int() &> only_lvalue{LvalueOnly{}};
    static_assert(std::is_invocable_v<decltype(only_lvalue)&>);
    static_assert(!std::is_invocable_v<decltype(only_lvalue)&&>);
    check(only_lvalue != nullptr, "ref-qualified move_only_function has target");
    check(only_lvalue() == 42, "ref-qualified move_only_function calls only on lvalues");

    std::move_only_function<int() const noexcept> const_noexcept{ConstNoexcept{}};
    static_assert(std::is_invocable_v<const decltype(const_noexcept)&>);
    static_assert(std::is_nothrow_invocable_v<const decltype(const_noexcept)&>);
    const auto& as_const = const_noexcept;
    check(as_const != nullptr, "const noexcept move_only_function has target");
    check(as_const() == 7, "const noexcept signature preserves const and noexcept invocation");
#else
    check(false, "std::move_only_function qualifiers require C++23 support");
#endif
}

void check_move_only_function()
{
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
    auto state = std::make_unique<int>(40);
    std::move_only_function<int()> task = [owned = std::move(state)]() { return *owned + 2; };
    static_assert(!std::is_copy_constructible_v<decltype(task)>);
    check(task != nullptr, "move_only_function has a target before call");
    check(task() == 42, "move_only_function invokes move-only target");

    std::move_only_function<int()> moved = std::move(task);
    check(moved() == 42, "moved move_only_function keeps target");
#else
    check(false, "std::move_only_function is required for this C++23 observation");
#endif
}

} // namespace

int main()
{
    check_invoke_forms();
    check_reference_wrapper_borrows();
    check_function_contracts();
    check_move_only_function();
    check_move_only_function_qualifiers();
}
