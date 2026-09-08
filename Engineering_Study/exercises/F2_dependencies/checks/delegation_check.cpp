#include <check.hpp>
#include "student.hpp"
#include "spy_provider/spy_provider.hpp"

namespace {
void check_delegates_one_call(int input) {
    f2_provider_spy::reset();
    const int result = student_use_dependency(input);
    check(result == 1000 + input, "student must return the provider result without recomputing or adjusting it");
    check(f2_provider_spy::call_count() == 1, "student must call f2_provider::compute_answer exactly once");
    check(f2_provider_spy::last_input() == input, "student must forward the current input to the provider");
}
}

int main() {
    check_delegates_one_call(20);
    check_delegates_one_call(-1);
    check_delegates_one_call(0);
    check_delegates_one_call(21);
}
