#include <record_ops.hpp>
#include <check.hpp>
#include <iostream>

namespace c04_record {
struct Partial {
    int shown{};
    int omitted{};
    bool operator==(const Partial&) const = default;
};
// Intentionally violates the registration's completeness obligation.
template<> struct schema<Partial> {
    static constexpr auto fields = std::tuple{field{"shown", &Partial::shown}};
};
}
int main() {
    using namespace c04_record;
    const Partial value{1, 9};
    const auto fields = implementation::encode_fields(value);
    const auto decoded = implementation::decode_fields<Partial>(fields);
    check(fields.size() == 1, "manual registration cannot discover an omitted member");
    check(decoded && decoded->shown == value.shown && decoded->omitted == 0 && *decoded != value,
          "violating metadata completeness loses an unregistered field");
    std::cout << "P1 observed the manual schema completeness boundary, not a verified reflection repair\n";
}
