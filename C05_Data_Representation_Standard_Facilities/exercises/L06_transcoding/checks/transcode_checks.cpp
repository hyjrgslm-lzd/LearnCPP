#include <utf_transcode.hpp>

#include <check.hpp>

#include <string>

int main()
{
    auto ascii = c05_ex::utf8_to_utf16("A\0B", 3);
    check(ascii && ascii->size() == 3 && (*ascii)[1] == u'\0', "preserves embedded nul");

    auto mixed = c05_ex::utf8_to_utf16(u8"z\u00a2\u20ac\U0001f642");
    check(mixed && mixed->size() == 5, "decodes bmp and supplementary scalar");

    auto round = c05_ex::utf16_to_utf8(*mixed);
    check(round && *round == std::string(reinterpret_cast<const char*>(u8"z\u00a2\u20ac\U0001f642")), "round trips strict text");

    const std::string overlong = "\xc0\xaf";
    auto bad_overlong = c05_ex::validate_utf8(overlong);
    check(!bad_overlong, "rejects overlong slash");
    check(bad_overlong.error().offset == 0, "overlong offset is sequence start");

    const std::string surrogate = "\xed\xa0\x80";
    auto bad_surrogate = c05_ex::validate_utf8(surrogate);
    check(!bad_surrogate && bad_surrogate.error().offset == 0, "rejects utf8 surrogate");

    const std::string truncated = "\xf0\x9f";
    auto bad_truncated = c05_ex::validate_utf8(truncated);
    check(!bad_truncated && bad_truncated.error().code == c05::Errc::incomplete_input, "rejects truncated input");

    const std::string bad_cont = "\xe2\x28\xa1";
    auto bad_continuation = c05_ex::validate_utf8(bad_cont);
    check(!bad_continuation && bad_continuation.error().offset == 0, "bad continuation reports sequence start");

    std::u16string bad_utf16{0xd83d};
    auto unpaired = c05_ex::utf16_to_utf8(bad_utf16);
    check(!unpaired && unpaired.error().unit == c05::OffsetUnit::utf16_code_unit, "utf16 error uses code unit offset");

    std::string huge(c05::max_package_bytes + 1, 'x');
    auto huge_utf8 = c05_ex::utf8_to_utf16(huge);
    check(!huge_utf8 && huge_utf8.error().unit == c05::OffsetUnit::byte, "utf8 input limit uses byte offset");

    std::u16string huge_utf16(c05::max_package_bytes / sizeof(char16_t) + 1, u'x');
    auto too_many_units = c05_ex::utf16_to_utf8(huge_utf16);
    check(!too_many_units && too_many_units.error().unit == c05::OffsetUnit::utf16_code_unit, "utf16 input limit uses code unit offset");
}
