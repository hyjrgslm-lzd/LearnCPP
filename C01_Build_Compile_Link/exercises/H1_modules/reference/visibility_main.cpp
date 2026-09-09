import visibility_boundary;

#include <check.hpp>

int main() {
    auto state = make_hidden();
    check(state.value == 42, "reachable hidden type member should be usable through auto");
    check(read_hidden(state) == 42, "exported function should accept the hidden type value");
}
