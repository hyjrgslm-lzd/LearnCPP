#include <c05/utf.hpp>
#include <check.hpp>

#include <string>
#include <string_view>
#include <vector>

static std::size_t count_code_points(std::string_view utf8)
{
    std::size_t count = 0;
    for (std::size_t i = 0; i < utf8.size();) {
        auto cp = c05::detail::decode_one_utf8(utf8, i);
        check(cp.has_value(), "input literals are strict UTF-8");
        ++count;
    }
    return count;
}

int main()
{
    const std::string composed = reinterpret_cast<const char*>(u8"\u00e9");
    const std::string decomposed = reinterpret_cast<const char*>(u8"e\u0301");
    check(c05::validate_utf8(composed).has_value(), "composed spelling is valid UTF-8");
    check(c05::validate_utf8(decomposed).has_value(), "decomposed spelling is valid UTF-8");
    check(composed != decomposed, "manifest identity keeps original UTF-8 bytes");
    check(count_code_points(composed) == 1, "NFC-style sample has one code point");
    check(count_code_points(decomposed) == 2, "NFD-style sample has two code points");

    const std::string family = reinterpret_cast<const char*>(u8"\U0001f469\u200d\U0001f467");
    check(count_code_points(family) == 3, "a displayed cluster can contain multiple code points");
    auto family16 = c05::utf8_to_utf16(family);
    check(family16 && family16->size() == 5, "UTF-16 offsets count surrogate pairs and joiner separately");

    const std::string turkish_i = reinterpret_cast<const char*>(u8"I\u0130\u0131i");
    check(count_code_points(turkish_i) == 4, "case rules operate on Unicode text, not bytes");

    std::vector<std::string> manifest_names{composed, decomposed};
    check(manifest_names[0] != manifest_names[1], "storage identifiers stay byte-stable even when display search may normalize");
}
