#include <array>
#include <check.hpp>
#include <string>
#include <vector>

namespace {

int dynamic_seed = 7;

int next_seed() {
    return ++dynamic_seed;
}

constinit int static_counter = 41;
int dynamic_counter = next_seed();

struct Defaults {
    int a;
    int b{};
};

struct Aggregate {
    int id;
    double weight;
};

struct CvBox {
    int value;
};

int mark(std::vector<int>& calls, int value) {
    calls.push_back(value);
    return value;
}

struct Pair {
    int left;
    int right;
};

} // namespace

int main() {
    int default_initialized;
    (void)sizeof(default_initialized);

    int value_initialized{};
    check(value_initialized == 0, "value initialization zeroes scalar");

    Defaults object{};
    check(object.a == 0 && object.b == 0, "value initialization zeroes aggregate members");

    Aggregate aggregate{.id = 3, .weight = 2.5};
    check(aggregate.id == 3 && aggregate.weight == 2.5, "designated aggregate initialization");

    std::array<int, 3> values{1, 2, 3};
    check(values[0] == 1 && values[2] == 3, "list initialization fills array");

    const CvBox const_box{5};
    volatile int volatile_value = const_box.value;
    check(volatile_value == 5, "cv-qualified object can be read through cv rules");

    check(static_counter == 41, "constinit object has constant initializer");
    check(dynamic_counter == 8, "dynamic initialization can run code before main");

    std::vector<int> calls;
    Pair pair{mark(calls, 1), mark(calls, 2)};
    check(pair.left == 1 && pair.right == 2, "aggregate stores evaluated values");
    check(calls == std::vector<int>({1, 2}), "brace initializer clauses are sequenced left to right");
}
