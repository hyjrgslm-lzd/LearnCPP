#include <config_parser.hpp>

#include <check.hpp>

#include <c05/types.hpp>

#include <string>

int main()
{
    auto defaults = c05_ex::parse_config("");
    check(defaults && defaults->package_file == "manifest.c05m" && defaults->display_zone == "UTC",
        "empty config keeps defaults");

    auto parsed = c05_ex::parse_config("\xef\xbb\xbf# c05\r\n package_file = pack.c05m \n display_zone = America/New_York \n");
    check(parsed && parsed->package_file == "pack.c05m", "parses package_file with bom crlf and trim");
    check(parsed && parsed->display_zone == "America/New_York", "parses display_zone");

    auto dup = c05_ex::parse_config("package_file=a.c05m\npackage_file=b.c05m\n");
    check(!dup && dup.error().code == c05::Errc::duplicate_field, "duplicate package_file");
    check(dup.error().line == 2, "duplicate reports one-based line");

    auto unknown = c05_ex::parse_config("answer=42\n");
    check(!unknown && unknown.error().field == "answer", "unknown key reports field");

    auto empty_value = c05_ex::parse_config("display_zone= \n");
    check(!empty_value && empty_value.error().code == c05::Errc::invalid_config, "empty value rejected");

    auto nested = c05_ex::parse_config("package_file=dir/pack.c05m\n");
    check(!nested, "package_file basename only");

    auto drive = c05_ex::parse_config("package_file=C:pack.c05m\n");
    check(!drive, "package_file rejects drive relative name");

    auto device = c05_ex::parse_config("package_file=CON.txt\n");
    check(!device, "package_file rejects windows device name");

    auto bad_tail = c05_ex::parse_config("package_file=pack.\n");
    check(!bad_tail, "package_file rejects illegal trailing dot");

    auto nul = c05_ex::parse_config(std::string{"display_zone=UT\0C\n", 18});
    check(!nul && nul.error().code == c05::Errc::invalid_config, "config rejects nul");
    check(nul.error().line == 1, "nul reports line");

    auto bad_utf8 = c05_ex::parse_config(std::string{"display_zone=\xc0\xaf\n", 16});
    check(!bad_utf8 && bad_utf8.error().code == c05::Errc::invalid_encoding, "config rejects invalid utf8");
    check(bad_utf8.error().line == 1, "invalid utf8 reports line");

    auto bad_line = c05_ex::parse_config("no_equals\n");
    check(!bad_line && bad_line.error().offset == 0 && bad_line.error().line == 1, "bad line reports offset and line");

    auto cr_inside = c05_ex::parse_config("display_zone=UT\rC\r\n");
    check(!cr_inside && cr_inside.error().offset == 15 && cr_inside.error().line == 1, "line-internal cr rejected at cr byte");

    auto bare_cr = c05_ex::parse_config("display_zone=UTC\r");
    check(!bare_cr && bare_cr.error().offset == 16 && bare_cr.error().line == 1, "bare trailing cr rejected");

    auto utf8_line = c05_ex::parse_config("package_file=a.c05m\ndisplay_zone=\xc0\xaf\n");
    check(!utf8_line && utf8_line.error().line == 2, "invalid utf8 reports later line");

    auto nul_line = c05_ex::parse_config(std::string{"package_file=a.c05m\ndisplay_zone=UT\0C\n", 38});
    check(!nul_line && nul_line.error().line == 2, "nul reports later line");

    auto superscript_device = c05_ex::parse_config("package_file=COM\xc2\xb9.txt\n");
    check(!superscript_device, "package_file rejects COM superscript device name");

    auto stem_space_device = c05_ex::parse_config("package_file=NUL .txt\n");
    check(!stem_space_device, "package_file rejects device stem with trailing space");
}
