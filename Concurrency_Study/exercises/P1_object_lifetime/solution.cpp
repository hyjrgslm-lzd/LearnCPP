#include "concurrency_study/exercise_check.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// Fixed storage keeps the observing destructor non-allocating and non-throwing.
struct lifetime_trace {
    std::array<int, 4> events{};
    std::size_t used = 0;
};
struct scope_object {
    int id;
    lifetime_trace& trace;
    scope_object(int id_, lifetime_trace& trace_) : id(id_), trace(trace_) {
        trace.events[trace.used++] = id;
    }
    ~scope_object() { trace.events[trace.used++] = -id; }
    scope_object(const scope_object&) = delete;
    scope_object& operator=(const scope_object&) = delete;
};

void part1_lifetime() {
    lifetime_trace trace;
    {
        scope_object first(1, trace);
        scope_object second(2, trace);
    }
    cs::check(trace.events == std::array{1, 2, -2, -1}, "reverse destruction");
    std::cout << "Part 1: construct 1, construct 2, destroy 2, destroy 1\n";
}

void part2_capture_and_move() {
    int value = 10;
    auto snapshot = [value]() mutable { return ++value; };
    auto borrowed = [&value] { return ++value; };
    value = 20;
    cs::check(snapshot() == 11 && value == 20, "mutable changes closure copy");
    cs::check(borrowed() == 21 && value == 21, "reference changes original");
    auto owner = std::make_unique<int>(42);
    auto task = [p = std::move(owner)] { return *p; };
    cs::check(!owner && task() == 42, "unique_ptr ownership transferred");
    static_assert(!std::is_copy_constructible_v<decltype(task)>);
    auto moved = std::move(task);
    cs::check(moved() == 42, "moved closure owns resource");
    // Do not invoke task: its unique_ptr has been moved out.
    std::cout << "Part 2: copy=11, original=21, moved owner=42\n";
}

struct counter {
    int value = 0;
    void add(int n) { value += n; }
};
void part3_invoke() {
    counter original;
    auto copy = original;
    std::invoke(&counter::add, copy, 3);
    cs::check(original.value == 0 && copy.value == 3, "copy is independent");
    std::invoke(&counter::add, std::ref(original), 7);
    cs::check(std::invoke(&counter::value, original) == 7, "member invocation");
    cs::check(std::invoke([](int n) { return n * 2; }, 21) == 42, "ordinary callable");
    std::cout << "Part 3: copy=3, referenced original=7, invoke lambda=42\n";
}

void part4_unwinding() {
    lifetime_trace trace;
    bool caught = false;
    try {
        scope_object first(1, trace);
        scope_object second(2, trace);
        throw std::runtime_error("unwind");
    } catch (const std::runtime_error&) { caught = true; }
    cs::check(caught && trace.events == std::array{1, 2, -2, -1}, "RAII on exception");
    std::cout << "Part 4: destroy 2, destroy 1, then handler\n";
}

void part5_tuple_ownership() {
    int number = 10;
    std::string text = "saved";
    auto owned = std::make_tuple(number, text);
    auto borrowed = std::tie(number, text);
    auto unwrapped = std::make_tuple(std::ref(number), std::cref(text));
    std::tuple<std::reference_wrapper<int>> wrapped{std::ref(number)};
    static_assert(std::is_same_v<decltype(owned), std::tuple<int, std::string>>);
    static_assert(std::is_same_v<decltype(borrowed), std::tuple<int&, std::string&>>);
    static_assert(std::is_same_v<decltype(unwrapped), std::tuple<int&, const std::string&>>);
    static_assert(std::is_same_v<decltype(std::get<0>(std::move(borrowed))), int&>);

    number = 20;
    text = "later";
    cs::check(std::get<0>(owned) == 10 && std::get<1>(owned) == "saved", "tuple owns copies");
    cs::check(std::get<0>(borrowed) == 20 && std::get<1>(borrowed) == "later", "tie borrows originals");
    cs::check(&std::get<0>(unwrapped) == &number && &std::get<1>(unwrapped) == &text,
              "make_tuple unwraps reference_wrapper into references");
    std::get<0>(wrapped).get() = 22;
    cs::check(number == 22 && std::get<0>(borrowed) == 22 && std::get<0>(unwrapped) == 22,
              "copied wrapper still refers to original");
    borrowed = std::make_tuple(30, std::string("assigned"));
    cs::check(number == 30 && text == "assigned", "tie assignment writes through references");
    cs::check(std::get<0>(owned) == 10 && std::get<1>(owned) == "saved", "owned snapshot unchanged");
    std::cout << "Part 5: owned=(10,saved); borrowed follows (20,later), wrapper writes 22, tie assigns 30\n";
}

void part6_saved_arguments() {
    int number = 10;
    int increment = 3;
    int calls = 0;
    auto add = [&calls](int n, int delta) { ++calls; return n + delta; };
    auto owned_call = [fn = add, args = std::make_tuple(number, increment)] {
        return std::apply(fn, args);
    };
    auto borrowed_call = [fn = add, args = std::tie(number, increment)] {
        return std::apply(fn, args);
    };
    number = 20;
    increment = 4;
    cs::check(calls == 0, "saving callable and arguments does not invoke");
    cs::check(owned_call() == 13 && borrowed_call() == 24 && calls == 2,
              "apply expands stored values or borrowed references at invocation");

    auto owner = std::make_unique<int>(37);
    auto arguments = std::make_tuple(std::move(owner), 5);
    auto consume = [](std::unique_ptr<int> p, int delta) {
        cs::check(static_cast<bool>(p), "consumer receives ownership");
        return *p + delta;
    };
    static_assert(!std::is_invocable_v<decltype(consume), std::unique_ptr<int>&, int>);
    static_assert(std::is_invocable_v<decltype(consume), std::unique_ptr<int>&&, int>);
    cs::check(!owner && *std::get<0>(arguments) == 37, "tuple takes ownership");
    auto task = [fn = consume, args = std::move(arguments)]() mutable {
        const int result = std::apply(fn, std::move(args));
        cs::check(!std::get<0>(args), "apply moves element into by-value parameter");
        return result;
    };
    static_assert(!std::is_copy_constructible_v<decltype(task)>);
    cs::check(!std::get<0>(arguments), "closure takes tuple's unique_ptr");
    cs::check(task() == 42, "saved move-only arguments consumed at later invocation");
    // This teaching task consumes its input. Do not invoke it a second time.
    std::cout << "Part 6: calls before invocation=0; owned=13, borrowed=24; moved argument result=42\n";
}

int main() {
    part1_lifetime();
    part2_capture_and_move();
    part3_invoke();
    part4_unwinding();
    part5_tuple_ownership();
    part6_saved_arguments();
    std::cout << "P1_reference OK\n";
}
