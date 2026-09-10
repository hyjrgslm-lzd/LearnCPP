#include <field_projection.hpp>

int main() {
    c04_projection::Person person{};
    (void)c04_projection::project<"id,,name">(person);
}
