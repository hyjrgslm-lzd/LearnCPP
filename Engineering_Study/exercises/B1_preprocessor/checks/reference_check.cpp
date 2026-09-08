#include "check.hpp"

int macro_value_from_a();
int macro_value_from_b();

int main()
{
    check(macro_value_from_a() == 118, "macro_a uses its own B1_LOCAL_OFFSET before inclusion");
    check(macro_value_from_b() == 129, "macro_b uses its own B1_LOCAL_OFFSET before inclusion");
    check(macro_value_from_a() != macro_value_from_b(), "two translation units keep separate macro state");
}
