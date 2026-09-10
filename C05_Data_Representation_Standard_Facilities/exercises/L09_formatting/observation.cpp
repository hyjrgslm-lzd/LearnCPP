#include <check.hpp>

#include <array>
#include <charconv>
#include <format>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>

struct Field {
    std::string name;
    int value{};
};

template<>
struct std::formatter<Field> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Field& field, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}={}", field.name, field.value);
    }
};

static std::string dynamic_format(std::string_view fmt, int value)
{
    return std::vformat(fmt, std::make_format_args(value));
}

int main()
{
    check(std::format("{}={}", "id", 7) == "id=7", "format builds owned text");

    std::array<char, 8> buffer{};
    auto truncated = std::format_to_n(buffer.begin(), 4, "size={}", 12345);
    check(truncated.size == 10, "format_to_n reports full output size");
    check(std::string(buffer.data(), 4) == "size", "format_to_n writes only bounded prefix");

    std::string out;
    std::format_to(std::back_inserter(out), "{}", Field{"count", 3});
    check(out == "count=3", "custom formatter is local formatting policy");

    check(dynamic_format("value={}", 5) == "value=5", "runtime format strings use vformat");
    bool threw = false;
    try {
        (void)dynamic_format("{", 5);
    } catch (const std::format_error&) {
        threw = true;
    }
    check(threw, "bad dynamic format is a runtime error");

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << 1234.5;
    check(stream.str().find("1234") == 0, "streams are locale-aware presentation tools");

    const std::string display = reinterpret_cast<const char*>(u8"e\u0301");
    check(display.size() == 3, "non-ASCII byte width is not display width");
}
