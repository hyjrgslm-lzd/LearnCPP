#include <field_projection.hpp>
#include <check.hpp>

int main() {
    c04_projection::Person person{7, true, "Ada"};
    auto refs = c04_projection::project<"name,id">(person);
    std::get<0>(refs) = "Grace";
    std::get<1>(refs) = 42;
    check(person.name == "Grace" && person.id == 42, "positive projection control compiles and runs");
}
