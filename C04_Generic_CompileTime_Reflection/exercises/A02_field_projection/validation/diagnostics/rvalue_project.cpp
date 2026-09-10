#include <field_projection.hpp>
#include <utility>

int main() {
    const c04_projection::Person person{};
    (void)c04_projection::project<"id">(std::move(person));
}
