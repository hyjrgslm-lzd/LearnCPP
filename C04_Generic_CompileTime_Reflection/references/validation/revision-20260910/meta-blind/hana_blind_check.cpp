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

#include <string>
#include <type_traits>

namespace hana = boost::hana;

struct HanaBlindRecord {
    int code{};
    std::string owner;
    bool closed{};
};

int main() {
    constexpr auto code_key = BOOST_HANA_STRING("code");
    constexpr auto owner_key = BOOST_HANA_STRING("owner");
    constexpr auto missing_key = BOOST_HANA_STRING("missing");

    constexpr auto type_schema = hana::make_map(
        hana::make_pair(code_key, hana::type_c<int>),
        hana::make_pair(owner_key, hana::type_c<std::string>),
        hana::make_pair(BOOST_HANA_STRING("closed"), hana::type_c<bool>));

    static_assert(hana::contains(type_schema, code_key));
    static_assert(!hana::contains(type_schema, missing_key));
    static_assert(hana::at_key(type_schema, owner_key) == hana::type_c<std::string>);
    static_assert(decltype(hana::is_just(hana::find(type_schema, code_key)))::value);
    static_assert(decltype(hana::is_nothing(hana::find(type_schema, missing_key)))::value);

    constexpr auto field_count = hana::integral_c<std::size_t, decltype(hana::length(type_schema))::value>;
    static_assert(field_count == hana::size_c<3>);

    HanaBlindRecord record{3, "ops", false};
    auto values = hana::make_map(
        hana::make_pair(code_key, &record.code),
        hana::make_pair(owner_key, &record.owner),
        hana::make_pair(BOOST_HANA_STRING("closed"), &record.closed));

    *hana::at_key(values, code_key) = 4;
    *hana::at_key(values, owner_key) = "qa";
    check(record.code == 4 && record.owner == "qa", "hana runtime values borrow the live object");

    auto owner = hana::find(values, owner_key);
    check(decltype(hana::is_just(owner))::value, "hana find returns optional-like result");
    check(**owner == "qa", "borrowed pointer reads the updated object");
}
