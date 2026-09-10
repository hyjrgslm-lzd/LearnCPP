#include <check.hpp>

#include <string>
#include <string_view>

static std::string owned_upper_ascii(std::string_view in)
{
    std::string out(in);
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return out;
}

int main()
{
    std::string text = "config";
    std::string_view view = text;
    check(view == "config", "string_view borrows existing storage");

    auto owned = owned_upper_ascii(view);
    text[0] = 'C';
    check(view == "Config", "view observes owner mutation");
    check(owned == "CONFIG", "returned string owns independent bytes");

    std::u8string u8 = u8"path";
    std::string_view bridge(reinterpret_cast<const char*>(u8.data()), u8.size());
    check(bridge == "path", "char8_t bytes can bridge to byte-oriented utf8 parser");
}
