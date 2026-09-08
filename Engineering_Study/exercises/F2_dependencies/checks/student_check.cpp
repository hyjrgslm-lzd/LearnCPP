#include <check.hpp>
#include "student.hpp"

int main() {
    check(student_use_dependency(20) == 42, "student should call the fixed provider without changing the fixture");
    check(student_use_dependency(-1) == 0, "student result must not be a constant 42");
    check(student_use_dependency(0) == 2, "student must preserve provider zero-input behavior");
    check(student_use_dependency(21) == 44, "student must forward the current input");
}
