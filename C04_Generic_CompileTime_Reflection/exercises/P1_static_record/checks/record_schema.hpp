#pragma once
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace c04_record {
struct external_name { const char* text; };
struct Person {
    int id{};
    bool active{};
    std::string name;
    bool operator==(const Person&) const = default;
};
struct Empty { bool operator==(const Empty&) const = default; };
struct Renamed {
#ifdef C04_RECORD_ANNOTATIONS
    [[= external_name{"id"}]]
#endif
    int code{};
    bool operator==(const Renamed&) const = default;
};
struct Unsupported { int* pointer{}; };
struct NoSchema { int number{}; };
struct DuplicateNames {
#ifdef C04_RECORD_ANNOTATIONS
    [[= external_name{"x"}]]
#endif
    int a{};
#ifdef C04_RECORD_ANNOTATIONS
    [[= external_name{"x"}]]
#endif
    int b{};
};
struct FieldValue {
    std::string name;
    std::string value;
    bool operator==(const FieldValue&) const = default;
};
using encoded_fields = std::vector<FieldValue>;
enum class field_error { unknown_field, duplicate_field, missing_field, invalid_value };

// These are supplied input metadata, not a traversal or codec implementation.
template<class Owner, class Member>
struct field {
    using owner_type = Owner;
    using member_type = Member;
    std::string_view name;
    Member Owner::* member;
};
template<class Owner, class Member>
field(std::string_view, Member Owner::*) -> field<Owner, Member>;
template<class T> struct schema;
template<> struct schema<Person> {
    static constexpr auto fields = std::tuple{field{"id", &Person::id},
        field{"active", &Person::active}, field{"name", &Person::name}};
};
template<> struct schema<Empty> { static constexpr auto fields = std::tuple{}; };
template<> struct schema<Renamed> {
    static constexpr auto fields = std::tuple{field{"id", &Renamed::code}};
};
template<> struct schema<Unsupported> {
    static constexpr auto fields = std::tuple{field{"pointer", &Unsupported::pointer}};
};
template<> struct schema<DuplicateNames> {
    static constexpr auto fields = std::tuple{field{"x", &DuplicateNames::a}, field{"x", &DuplicateNames::b}};
};
}
