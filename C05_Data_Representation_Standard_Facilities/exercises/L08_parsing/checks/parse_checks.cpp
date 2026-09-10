#include <text_parse.hpp>

#include <check.hpp>

#include <c05/types.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

int main()
{
    auto id = c05_ex::parse_u32("42", "id");
    check(id && *id == 42, "parses decimal u32");

    auto max_u32 = c05_ex::parse_u32("4294967295", "id");
    check(max_u32 && *max_u32 == std::numeric_limits<std::uint32_t>::max(), "parses u32 max");

    auto bad_unsigned_sign = c05_ex::parse_u32("-1", "id");
    check(!bad_unsigned_sign && bad_unsigned_sign.error().code == c05::Errc::invalid_number, "rejects signed text for u32");
    check(bad_unsigned_sign.error().offset == 0 && bad_unsigned_sign.error().field == "id", "u32 badarg owns field at byte zero");

    auto u32_over = c05_ex::parse_u32("4294967296", "id");
    check(!u32_over && u32_over.error().code == c05::Errc::out_of_range, "rejects u32 overflow");

    auto no_trim = c05_ex::parse_u32(" 42", "id");
    check(!no_trim && no_trim.error().code == c05::Errc::invalid_number, "does not trim leading spaces");

    auto trailing = c05_ex::parse_u32("42x", "id");
    check(!trailing && trailing.error().code == c05::Errc::trailing_data, "rejects trailing characters");
    check(trailing.error().offset == 2 && trailing.error().unit == c05::OffsetUnit::byte, "trailing offset is first unconsumed byte");

    auto min_i64 = c05_ex::parse_i64("-9223372036854775808", "mtime");
    check(min_i64 && *min_i64 == std::numeric_limits<std::int64_t>::min(), "parses i64 min");
    auto max_i64 = c05_ex::parse_i64("9223372036854775807", "mtime");
    check(max_i64 && *max_i64 == std::numeric_limits<std::int64_t>::max(), "parses i64 max");
    auto i64_over = c05_ex::parse_i64("9223372036854775808", "mtime");
    check(!i64_over && i64_over.error().code == c05::Errc::out_of_range, "rejects i64 overflow");

    auto finite = c05_ex::parse_finite_double("0.125", "ratio");
    check(finite && *finite == 0.125, "parses finite double");
    auto sci = c05_ex::parse_finite_double("1e3", "ratio");
    check(sci && *sci == 1000.0, "parses exponent form");
    auto double_trailing = c05_ex::parse_finite_double("1.5ms", "ratio");
    check(!double_trailing && double_trailing.error().code == c05::Errc::trailing_data, "rejects double trailing characters");
    auto nonfinite = c05_ex::parse_finite_double("inf", "ratio");
    check(!nonfinite && nonfinite.error().code == c05::Errc::invalid_value, "rejects non-finite double");
    auto huge = c05_ex::parse_finite_double("1e9999", "ratio");
    check(!huge && (huge.error().code == c05::Errc::out_of_range || huge.error().code == c05::Errc::invalid_value), "rejects huge double");
}
