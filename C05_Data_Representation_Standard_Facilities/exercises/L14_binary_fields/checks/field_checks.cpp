#include <bounded_field.hpp>

#include <check.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

static std::vector<std::byte> field(std::initializer_list<unsigned char> payload)
{
    std::vector<std::byte> out;
    auto n = static_cast<std::uint32_t>(payload.size());
    out.push_back(static_cast<std::byte>((n >> 24) & 0xff));
    out.push_back(static_cast<std::byte>((n >> 16) & 0xff));
    out.push_back(static_cast<std::byte>((n >> 8) & 0xff));
    out.push_back(static_cast<std::byte>(n & 0xff));
    for (auto b : payload) {
        out.push_back(static_cast<std::byte>(b));
    }
    return out;
}

int main()
{
    auto valid = field({'A', 0, 0xe2, 0x82, 0xac});
    std::size_t cursor = 0;
    auto text = c05_ex::read_utf8_field(valid, cursor);
    check(text && *text == std::string("A\0\xe2\x82\xac", 5), "reads valid utf8 field and preserves nul");
    check(cursor == valid.size(), "commits cursor after full success");

    std::vector<std::byte> prefixed{std::byte{0x42}};
    prefixed.insert(prefixed.end(), valid.begin(), valid.end());
    cursor = 1;
    text = c05_ex::read_utf8_field(prefixed, cursor);
    check(text && *text == std::string("A\0\xe2\x82\xac", 5), "reads field from nonzero cursor");
    check(cursor == prefixed.size(), "commits nonzero cursor to end of field");

    auto empty = field({});
    cursor = 0;
    text = c05_ex::read_utf8_field(empty, cursor);
    check(text && text->empty() && cursor == 4, "accepts empty generic text field");

    auto trailing = field({'o', 'k'});
    trailing.push_back(std::byte{0x99});
    cursor = 0;
    text = c05_ex::read_utf8_field(trailing, cursor);
    check(text && *text == "ok" && cursor == 6, "reads one field without consuming following bytes");

    std::vector<std::byte> too_short{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{3}, std::byte{'x'}};
    cursor = 0;
    auto short_result = c05_ex::read_utf8_field(too_short, cursor);
    check(!short_result, "rejects truncated payload");
    check(cursor == 0, "truncated payload keeps cursor");

    auto complete = field({'x', 'y', 'z'});
    for (std::size_t n = 0; n != complete.size(); ++n) {
        std::vector<std::byte> prefix(complete.begin(), complete.begin() + static_cast<std::ptrdiff_t>(n));
        cursor = 0;
        auto truncated = c05_ex::read_utf8_field(prefix, cursor);
        check(!truncated, "rejects every truncate point");
        check(cursor == 0, "every truncate point keeps cursor");
    }

    std::vector<std::byte> too_long{std::byte{0}, std::byte{0}, std::byte{0x10}, std::byte{0x01}};
    cursor = 0;
    auto long_result = c05_ex::read_utf8_field(too_long, cursor);
    check(!long_result && long_result.error().code == c05::Errc::limit_exceeded, "rejects oversized length before allocation");
    check(cursor == 0, "oversized length keeps cursor");

    auto invalid = field({0xe2, 0x28, 0xa1});
    cursor = 0;
    auto invalid_result = c05_ex::read_utf8_field(invalid, cursor);
    check(!invalid_result, "rejects invalid utf8");
    check(invalid_result.error().offset == 4, "invalid utf8 offset maps to whole buffer byte coordinate");
    check(cursor == 0, "invalid utf8 keeps cursor");

    cursor = valid.size() + 1;
    auto past_end = c05_ex::read_utf8_field(valid, cursor);
    check(!past_end, "rejects cursor past input size");
    check(cursor == valid.size() + 1, "cursor past size keeps cursor");

    cursor = static_cast<std::size_t>(-1);
    auto max_cursor = c05_ex::read_utf8_field(valid, cursor);
    check(!max_cursor, "rejects size_max cursor");
    check(cursor == static_cast<std::size_t>(-1), "size_max cursor keeps cursor");
}
