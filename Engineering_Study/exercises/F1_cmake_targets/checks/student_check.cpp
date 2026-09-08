#include <check.hpp>
#include "student.hpp"

int main() {
#ifndef F1_REQUIRE_EXPLICIT_RESULT
    check(false, "student target did not receive the linked INTERFACE requirement");
#endif
    check(student_add_scaled(20, 1) == 42, "student_add_scaled must implement the target contract");
    check(student_add_scaled(0, 0) == 0, "student result must not be a constant 42");
    check(student_add_scaled(-2, 3) == 2, "student implementation must use both inputs");
}
