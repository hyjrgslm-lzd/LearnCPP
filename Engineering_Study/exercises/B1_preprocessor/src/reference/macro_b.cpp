#define B1_LOCAL_OFFSET 22
#include "generated_value.hpp"
#include "pragma_once_example.hpp"

int macro_value_from_b()
{
    return generated_value() + pragma_once_example_value();
}
