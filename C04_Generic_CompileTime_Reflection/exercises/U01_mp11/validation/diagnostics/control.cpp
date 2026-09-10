#include <mp11_schema_tools.hpp>
#include <projection_schema.hpp>

using ok = c04_mp11::required_field_t<c04_projection::Person, "id">;

int main() {
    (void)sizeof(ok);
}
