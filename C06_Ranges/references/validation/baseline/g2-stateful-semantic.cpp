#include <concepts>
#include <iostream>

struct Stateful {
    int delta = 10;
    int calls = 0;
    int operator()(int value) { return value + delta + ++calls; }
};

// Concepts test syntax; this assertion cannot verify the semantic requirements.
static_assert(std::regular_invocable<Stateful&, int>);
int main() {
    Stateful function;
    const int first = function(1);
    const int second = function(1);
    std::cout << "same argument results=" << first << ',' << second
              << "; mutated function calls=" << function.calls << '\n';
    return first == 12 && second == 13 && function.calls == 2 ? 0 : 1;
}
