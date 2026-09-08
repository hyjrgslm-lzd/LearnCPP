#include <check.hpp>
#include <f2_provider/provider.hpp>

int main() {
    check(f2_provider::api_version() == 100, "fixed provider version must be visible");
    check(f2_provider::compute_answer(20) == 42, "provider fixture contract changed");
    check(f2_provider::compute_answer(-1) == 0, "provider negative input contract changed");
    check(f2_provider::compute_answer(0) == 2, "provider zero input contract changed");
    check(f2_provider::compute_answer(21) == 44, "provider must consume the supplied input");
}
