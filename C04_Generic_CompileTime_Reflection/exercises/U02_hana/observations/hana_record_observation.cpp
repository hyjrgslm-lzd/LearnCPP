#include <check.hpp>
#include <projection_schema.hpp>

#include <boost/hana/at_key.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/type.hpp>

#include <iostream>
#include <string>
#include <type_traits>

namespace hana = boost::hana;
using namespace hana::literals;

int main() {
    using c04_projection::Person;

    constexpr auto id_key = BOOST_HANA_STRING("id");
    constexpr auto name_key = BOOST_HANA_STRING("name");
    constexpr auto age_key = BOOST_HANA_STRING("age");

    constexpr auto type_schema = hana::make_map(
        hana::make_pair(id_key, hana::type_c<int>),
        hana::make_pair(name_key, hana::type_c<std::string>),
        hana::make_pair(BOOST_HANA_STRING("active"), hana::type_c<bool>));

    static_assert(hana::contains(type_schema, id_key));
    static_assert(!hana::contains(type_schema, age_key));
    static_assert(hana::at_key(type_schema, id_key) == hana::type_c<int>);
    static_assert(decltype(hana::is_just(hana::find(type_schema, name_key)))::value);

    constexpr auto field_count = hana::integral_c<std::size_t, decltype(hana::length(type_schema))::value>;
    static_assert(field_count == hana::size_c<3>);

    Person person{7, true, "Ada"};
    auto values = hana::make_map(
        hana::make_pair(id_key, &person.id),
        hana::make_pair(name_key, &person.name),
        hana::make_pair(BOOST_HANA_STRING("active"), &person.active));

    *hana::at_key(values, id_key) = 42;
    *hana::at_key(values, name_key) = "Grace";
    check(person.id == 42 && person.name == "Grace", "hana map can hold borrowed runtime values");

    auto found = hana::find(values, name_key);
    check(decltype(hana::is_just(found))::value, "hana::find returns an optional-like compile-time branch");
    check(**found == "Grace", "found runtime pointer still reads the live object");

    std::cout << "U02 Hana record observation passed\n";
}
