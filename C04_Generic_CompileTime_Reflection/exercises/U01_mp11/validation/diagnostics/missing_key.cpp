#include <mp11_schema_tools.hpp>
#include <projection_schema.hpp>

using missing = c04_mp11::required_field_t<c04_projection::Person, "not_a_field">;

int main() {
    (void)sizeof(missing);
}
