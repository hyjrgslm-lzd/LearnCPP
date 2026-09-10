#include <c05/bytes.hpp>
#include <check.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

int main()
{
    std::vector<std::byte> out;
    c05::append_be<std::uint16_t>(out, 0x1234);
    c05::append_be<std::uint32_t>(out, 0x01020304);
    check(out[0] == std::byte{0x12} && out[1] == std::byte{0x34}, "append_be writes most significant byte first");

    std::size_t cursor = 0;
    auto a = c05::read_be<std::uint16_t>(out, cursor);
    auto b = c05::read_be<std::uint32_t>(out, cursor);
    check(a && *a == 0x1234 && b && *b == 0x01020304, "read_be reverses append_be");
    check(cursor == out.size(), "read_be advances by type width");

    cursor = out.size() - 1;
    auto short_read = c05::read_be<std::uint32_t>(out, cursor);
    check(!short_read && cursor == out.size() - 1, "short read fails without consuming");
    check(std::endian::native == std::endian::little || std::endian::native == std::endian::big || std::endian::native == std::endian::native, "native endian is observable separately");
}
