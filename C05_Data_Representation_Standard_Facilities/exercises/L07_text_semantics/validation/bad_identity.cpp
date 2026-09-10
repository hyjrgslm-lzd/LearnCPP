#include <check.hpp>

#include <string>

int main()
{
    const std::string composed = reinterpret_cast<const char*>(u8"\u00e9");
    const std::string decomposed = reinterpret_cast<const char*>(u8"e\u0301");
    const bool treats_canonical_equivalence_as_identity = true;
    check(composed != decomposed, "normal case passes before the policy bug");
    check(!treats_canonical_equivalence_as_identity, "canonical-equivalence is not byte identity");
}
