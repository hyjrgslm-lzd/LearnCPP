#include <c05/utf.hpp>
#include <check.hpp>

#include <string>

int main()
{
    std::string euro = reinterpret_cast<const char*>(u8"\u20ac");
    check(euro.size() == 3, "U+20AC is three UTF-8 code units");
    auto euro16 = c05::utf8_to_utf16(euro);
    check(euro16 && euro16->size() == 1, "U+20AC is one UTF-16 code unit");

    std::string smile = reinterpret_cast<const char*>(u8"\U0001f642");
    auto smile16 = c05::utf8_to_utf16(smile);
    check(smile.size() == 4 && smile16 && smile16->size() == 2, "supplementary scalar uses four UTF-8 bytes and two UTF-16 units");

    std::string bom = "\xef\xbb\xbf" "text";
    auto bom16 = c05::utf8_to_utf16(bom);
    check(bom16 && (*bom16)[0] == 0xfeff, "BOM is preserved by generic transcoding");
}
