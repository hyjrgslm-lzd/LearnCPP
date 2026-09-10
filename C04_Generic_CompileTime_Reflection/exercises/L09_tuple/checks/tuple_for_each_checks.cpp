#include <check.hpp>
#include <tuple_for_each.hpp>

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

struct StatefulCallback {
    int calls = 0;
    template<class T>
    void operator()(T&&) {
        ++calls;
    }
};

int main() {
    std::tuple<> empty;
    StatefulCallback empty_cb;
    c04::for_each_tuple(empty, empty_cb);
    check(empty_cb.calls == 0, "empty tuple must perform zero callback calls");

    std::tuple<int, int> ordered{10, 20};
    std::array<int, 2> seen{};
    int index = 0;
    c04::for_each_tuple(ordered, [&](int& value) { seen[index++] = value; });
    check((seen == std::array{10, 20}), "tuple traversal must preserve left-to-right order");

    StatefulCallback stateful;
    c04::for_each_tuple(ordered, stateful);
    check(stateful.calls == 2, "callback object must be reused as one lvalue");

    c04::for_each_tuple(ordered, [](int& value) { value += 1; });
    check(std::get<0>(ordered) == 11 && std::get<1>(ordered) == 21, "tuple traversal preserves mutable references");

    const std::tuple<int> const_tuple{7};
    bool saw_const = false;
    c04::for_each_tuple(const_tuple, [&](const int& value) {
        saw_const = std::is_const_v<std::remove_reference_t<decltype(value)>> && value == 7;
    });
    check(saw_const, "const tuple element is visited as const reference");

    std::tuple<std::unique_ptr<int>> move_only{std::make_unique<int>(5)};
    bool moved = false;
    c04::for_each_tuple(std::move(move_only), [&](std::unique_ptr<int>&& value) {
        auto local = std::move(value);
        moved = local && *local == 5;
    });
    check(moved && !std::get<0>(move_only), "rvalue tuple preserves move-only rvalue element access");

    std::vector<int> side_effects;
    bool threw = false;
    try {
        c04::for_each_tuple(std::tuple{1, 2, 3}, [&](int value) {
            side_effects.push_back(value);
            if (value == 2) {
                throw std::runtime_error("stop");
            }
        });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "callback exception propagates");
    check((side_effects == std::vector{1, 2}), "exception stops later elements but does not roll back prior side effects");
    std::cout << "L09_tuple checks OK\n";
}
