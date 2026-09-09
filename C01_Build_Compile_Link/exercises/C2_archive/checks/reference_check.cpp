#include "archive_value.hpp"
#include "check.hpp"
int main()
{
    check(archive_value() == 42, "static archive extracts the member that defines archive_value");
    check(archive_value() + 1 == 43, "consumer observes the linked function result, not a skipped call");
}
