#include "mp11_schema_tools.blind.hpp"

#include <check.hpp>

#include <boost/mp11.hpp>
#include <boost/mp11/map.hpp>

#include <concepts>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace mp11 = boost::mp11;

struct BlindRecord {
    int number{};
    std::string label;
    bool enabled{};
};

namespace c04_projection {
template<>
struct schema<::BlindRecord> {
    static constexpr auto fields = std::tuple{
        field<"number", &::BlindRecord::number>{},
        field<"label", &::BlindRecord::label>{},
        field<"enabled", &::BlindRecord::enabled>{}
    };
};
}

template<class Pair>
using make_const_value_pair = mp11::mp_list<c04_mp11::field_key_t<Pair>, const c04_mp11::field_value_type_t<Pair>>;

template<class Pair>
using is_string_field = std::is_same<c04_mp11::field_value_type_t<Pair>, std::string>;

template<class A, class B>
using pair_names = mp11::mp_list<c04_mp11::field_key_t<A>, c04_mp11::field_key_t<B>>;

template<class Key, class Value>
using pointer_value = std::add_pointer_t<Value>;

struct DescribeField {
    template<class Pair>
    std::string operator()() const {
        return std::string(c04_mp11::field_name_v<c04_mp11::field_key_t<Pair>>) + ":" +
            std::string(c04_mp11::field_value_label_v<Pair>);
    }
};

int main() {
    using c04_projection::Order;
    using c04_projection::Person;

    using person_map = c04_mp11::schema_map_t<Person>;
    static_assert(mp11::mp_is_map<person_map>::value);
    static_assert(c04_mp11::field_count_v<Person> == 3);
    static_assert(c04_mp11::keys_are_unique_v<Person>);
    static_assert(std::same_as<c04_mp11::find_field_t<Person, "missing">, void>);
    static_assert(c04_mp11::has_field_v<Person, "id">);
    static_assert(!c04_mp11::has_field_v<Person, "missing">);

    using id_pair = c04_mp11::required_field_t<Person, "id">;
    using name_pair = c04_mp11::required_field_t<Person, "name">;
    static_assert(std::same_as<c04_mp11::field_value_type_t<id_pair>, int>);
    static_assert(std::same_as<c04_mp11::field_value_type_t<name_pair>, std::string>);

    using duplicate_id = mp11::mp_list<c04_mp11::key<"id">, c04_mp11::member_c<&Person::active>>;
    using inserted = c04_mp11::insert_field_t<person_map, duplicate_id>;
    static_assert(std::same_as<c04_mp11::field_value_type_t<c04_mp11::required_pair_t<inserted, c04_mp11::key<"id">>>, int>);

    using replaced = c04_mp11::replace_field_t<person_map, duplicate_id>;
    static_assert(std::same_as<c04_mp11::field_value_type_t<c04_mp11::required_pair_t<replaced, c04_mp11::key<"id">>>, bool>);

    using small_map = mp11::mp_list<mp11::mp_list<c04_mp11::key<"id">, int>>;
    using updated = c04_mp11::update_field_t<small_map, mp11::mp_list<c04_mp11::key<"id">, void>, pointer_value>;
    static_assert(std::same_as<mp11::mp_second<c04_mp11::required_pair_t<updated, c04_mp11::key<"id">>>, int*>);

    using const_pairs = c04_mp11::transform_fields_t<make_const_value_pair, person_map>;
    static_assert(std::same_as<mp11::mp_second<mp11::mp_first<const_pairs>>, const int>);

    using string_fields = c04_mp11::filter_fields_t<is_string_field, person_map>;
    static_assert(mp11::mp_size<string_fields>::value == 1);
    static_assert(std::same_as<c04_mp11::field_key_t<mp11::mp_first<string_fields>>, c04_mp11::key<"name">>);

    using unique = c04_mp11::unique_types_t<mp11::mp_list<int, bool, int, std::string, bool>>;
    static_assert(std::same_as<unique, mp11::mp_list<int, bool, std::string>>);

    using product = c04_mp11::product_t<pair_names, string_fields, c04_mp11::schema_map_t<Order>>;
    static_assert(mp11::mp_size<product>::value == 3);

    struct Then { using type = int; };
    struct Else {};
    static_assert(std::same_as<c04_mp11::lazy_provider_t<true, Then, Else>, int>);

    Person person{7, true, "Ada"};
    auto& id = c04_mp11::get_by_key<"id">(person);
    static_assert(std::same_as<decltype(id), int&>);
    id = 8;
    check(person.id == 8, "key lookup returns writable member reference");

    check(c04_mp11::visit_field_type<Person>(0, DescribeField{}) == "id:int", "person index 0");
    check(c04_mp11::visit_field_type<Person>(2, DescribeField{}) == "name:string", "person index 2");

    BlindRecord record{11, "ops", false};
    c04_mp11::get_by_key<"label">(record) = "qa";
    check(record.label == "qa", "unknown record key lookup");
    check(c04_mp11::visit_field_type<BlindRecord>(2, DescribeField{}) == "enabled:bool", "unknown record runtime dispatch");

    bool threw = false;
    try {
        (void)c04_mp11::visit_field_type<Person>(9, DescribeField{});
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "runtime dispatch rejects out-of-range index before mp_with_index");
}
