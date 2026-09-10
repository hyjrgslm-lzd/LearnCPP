#include <check.hpp>
#include <projection_schema.hpp>

#include <iostream>
#include <string>
#include <tuple>

namespace {
auto copy_id_name(c04_projection::Person person) {
    return std::tuple{person.id, person.name};
}

auto schema_order_even_when_name_first(c04_projection::Person& person) {
    return std::tie(person.id, person.name);
}
}

int main() {
    c04_projection::Person person{7, true, "Ada"};
    auto copied = copy_id_name(person);
    std::get<0>(copied) = 9;
    check(person.id == 7, "copying fields does not implement projection writeback");

    auto wrong_order = schema_order_even_when_name_first(person);
    check(std::is_same_v<decltype(wrong_order), std::tuple<int&, std::string&>>,
        "schema-order tuple has the wrong type for project<\"name,id\">");
    std::cout << "projection counterexamples observed\n";
}
