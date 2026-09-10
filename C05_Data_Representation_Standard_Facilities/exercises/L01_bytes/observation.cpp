#include <check.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

struct PacketWord {
    std::uint16_t tag;
    std::uint16_t flags;
};

int main()
{
    static_assert(std::is_trivially_copyable_v<PacketWord>);
    PacketWord word{0x1234, 0x00ff};
    auto bytes = std::bit_cast<std::array<std::byte, sizeof(PacketWord)>>(word);
    auto round_trip = std::bit_cast<PacketWord>(bytes);
    check(round_trip.tag == word.tag && round_trip.flags == word.flags, "bit_cast copies object representation");
    check(bytes.size() == sizeof(PacketWord), "byte view has object size");

    bytes[0] = std::byte{0};
    auto changed = std::bit_cast<PacketWord>(bytes);
    check(changed.tag != word.tag || changed.flags != word.flags, "changing bytes changes represented value on this type");
}
