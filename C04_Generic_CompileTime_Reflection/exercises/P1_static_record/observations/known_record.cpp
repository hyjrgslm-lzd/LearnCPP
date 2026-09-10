#include <check.hpp>
#include <record_schema.hpp>
#include <iostream>

c04_record::encoded_fields encode_known_person(const c04_record::Person& person) {
    return {{"id", std::to_string(person.id)}, {"active", person.active ? "true" : "false"}, {"name", person.name}};
}
int main() {
    const c04_record::Person person{7, true, "Ada"};
    check(encode_known_person(person) == c04_record::encoded_fields{{"id", "7"}, {"active", "true"}, {"name", "Ada"}},
          "known Person baseline preserves every declared field");
    std::cout << "P1 known-type encoding baseline passed\n";
}
