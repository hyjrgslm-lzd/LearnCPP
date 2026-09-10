#include <check.hpp>
#include <my_enumerate_view.hpp>
#include <ranges>

struct MoveOnlyValue {
    int value;
    explicit MoveOnlyValue(int x) : value(x) {}
    MoveOnlyValue(MoveOnlyValue&&) = default;
    MoveOnlyValue& operator=(MoveOnlyValue&&) = default;
    MoveOnlyValue(const MoveOnlyValue&) = delete;
    MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
    bool operator==(const MoveOnlyValue&) const = default;
};

int main() {
    // Value-based equality makes this pure factory a valid regular_invocable.
    auto input = std::views::iota(1, 4)
        | std::views::transform([](int x) { return MoveOnlyValue{x}; });
    auto indexed = c06_g3::my_enumerate(input);
    auto first = *indexed.begin();
    auto third = indexed.begin()[2];
    check(first.first == 0 && first.second.value == 1, "move-only prvalue dereference");
    check(third.first == 2 && third.second.value == 3, "move-only prvalue indexing");
}
