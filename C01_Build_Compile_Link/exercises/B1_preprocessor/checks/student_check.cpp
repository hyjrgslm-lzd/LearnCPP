#include "check.hpp"
#include "student_value.hpp"
int main()
{
    check(b1_student::configured_value() == 123, "student configured_value must use the requested preprocessing-safe value");
}
