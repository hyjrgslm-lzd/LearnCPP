#include "student.hpp"

#include <f2_provider/provider.hpp>

int student_use_dependency(int input) {
    return f2_provider::compute_answer(input) - 2;
}
