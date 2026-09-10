#include <check.hpp>
#include <projection_schema.hpp>

#include <iostream>
#include <tuple>

namespace {
int& get_known_id(c04_projection::Person& person) { return person.id; }
auto known_name_id(c04_projection::Person& person) {
    return std::tie(person.name, person.id);
}
}

int main() {
    c04_projection::Person person{7, true, "Ada"};
    get_known_id(person) = 8;
    auto picked = known_name_id(person);
    std::get<0>(picked) = "Grace";
    std::get<1>(picked) = 42;
    check(person.id == 42 && person.name == "Grace", "known projection baseline writes through references");
    std::cout << "known projection baseline OK\n";
}
