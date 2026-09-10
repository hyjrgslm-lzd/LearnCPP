#include <field_projection.hpp>

struct ExternalRecord {
    int id{};
};

int main() {
    ExternalRecord record{};
    (void)c04_projection::project<"id">(record);
}
