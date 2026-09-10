#include <compiletime_values.hpp>

#include <array>

consteval auto escaped_scratch_pointer() {
    auto scratch = std::array{c04_values::make_row("hp", 1)};
    return c04_values::sort_by_key(scratch).data();
}

constexpr auto bad_pointer = escaped_scratch_pointer();
constexpr int bad_value = bad_pointer->value;

int main() {
    return bad_value;
}
