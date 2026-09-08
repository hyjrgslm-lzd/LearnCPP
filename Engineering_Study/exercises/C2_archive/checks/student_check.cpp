#include "archive_value.hpp"
#include "check.hpp"
int main()
{
    check(archive_value() == 42, "student archive_value must provide the linked definition returning 42");
}
