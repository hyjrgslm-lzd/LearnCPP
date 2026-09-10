#include <check.hpp>

#include <boost/hana/at_key.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/type.hpp>

#include <functional>
#include <iostream>
#include <string>

namespace hana = boost::hana;

struct SensorReading {
    int channel{};
    double voltage{};
    std::string unit;
    bool saturated{};
};

int main() {
    constexpr auto channel_key = BOOST_HANA_STRING("channel");
    constexpr auto voltage_key = BOOST_HANA_STRING("voltage");
    constexpr auto unit_key = BOOST_HANA_STRING("unit");
    constexpr auto saturated_key = BOOST_HANA_STRING("saturated");
    constexpr auto missing_key = BOOST_HANA_STRING("calibration");

    constexpr auto type_schema = hana::make_map(
        hana::make_pair(channel_key, hana::type_c<int>),
        hana::make_pair(voltage_key, hana::type_c<double>),
        hana::make_pair(unit_key, hana::type_c<std::string>),
        hana::make_pair(saturated_key, hana::type_c<bool>));

    static_assert(hana::contains(type_schema, channel_key));
    static_assert(!hana::contains(type_schema, missing_key));
    static_assert(hana::at_key(type_schema, voltage_key) == hana::type_c<double>);
    static_assert(hana::at_key(type_schema, unit_key) == hana::type_c<std::string>);
    static_assert(decltype(hana::is_nothing(hana::find(type_schema, missing_key)))::value);
    static_assert(hana::length(type_schema) == hana::size_c<4>);

    SensorReading reading{2, 3.3, "V", false};

    auto copied_values = hana::make_map(
        hana::make_pair(channel_key, reading.channel),
        hana::make_pair(voltage_key, reading.voltage),
        hana::make_pair(unit_key, reading.unit),
        hana::make_pair(saturated_key, reading.saturated));

    hana::at_key(copied_values, channel_key) = 4;
    hana::at_key(copied_values, unit_key) = "mV";
    check(reading.channel == 2 && reading.unit == "V",
          "copy map updates do not update the source object");

    auto borrowed_values = hana::make_map(
        hana::make_pair(channel_key, std::ref(reading.channel)),
        hana::make_pair(voltage_key, std::ref(reading.voltage)),
        hana::make_pair(unit_key, std::ref(reading.unit)),
        hana::make_pair(saturated_key, std::ref(reading.saturated)));

    hana::at_key(borrowed_values, channel_key).get() = 8;
    hana::at_key(borrowed_values, voltage_key).get() = 5.0;
    hana::at_key(borrowed_values, unit_key).get() = "mV";
    check(reading.channel == 8 && reading.voltage == 5.0 && reading.unit == "mV",
          "borrowed map updates write through std::ref");

    auto found_unit = hana::find(borrowed_values, unit_key);
    check(decltype(hana::is_just(found_unit))::value,
          "hana::find reports existing keys without making at_key a precondition");
    check((*found_unit).get() == "mV", "hana::find returns the borrowed runtime value");

    auto missing_value = hana::find(borrowed_values, missing_key);
    check(decltype(hana::is_nothing(missing_value))::value,
          "hana::find reports absent keys without a compile error");

    const auto const_borrowed_values = borrowed_values;
    hana::at_key(const_borrowed_values, saturated_key).get() = true;
    check(reading.saturated, "const hana map does not make a referenced object const");

    std::cout << "U02 Hana migration solution passed\n";
}
