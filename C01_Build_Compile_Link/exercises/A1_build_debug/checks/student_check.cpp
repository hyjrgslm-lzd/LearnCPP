#include "check.hpp"
#include "debug_story.hpp"

int main()
{
    check(a1_student::compute_answer(19) == 42, "student compute_answer(19) must return 42");
    check(a1_student::compute_answer(20) == 44, "student implementation must use the input, not a constant");
}
