#include <check.hpp>

#include <bit>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <type_traits>

struct Packet {
    char tag;
    int value;
};

struct Empty {};

struct UsesEmptyBase : Empty {
    int value;
};

struct NonTrivial {
    int value;
    ~NonTrivial() {}
};

int main()
{
    Packet packets[3]{{'a', 1}, {'b', 2}, {'c', 3}};
    auto* begin = packets;
    auto* end = packets + 3;
    check(end - begin == 3, "array one-past pointer must stay in the same array");
    check(reinterpret_cast<std::byte*>(&packets[1]) - reinterpret_cast<std::byte*>(&packets[0])
              == static_cast<std::ptrdiff_t>(sizeof(Packet)),
          "array stride equals sizeof(element)");

    Packet copy{};
    std::memcpy(&copy, &packets[0], sizeof(Packet));
    check(copy.tag == 'a' && copy.value == 1, "trivially copyable object restored by memcpy");

    const auto bits = std::bit_cast<unsigned char>('A');
    check(bits == static_cast<unsigned char>('A'), "bit_cast creates a value, not an alias");

    static_assert(std::is_standard_layout_v<Packet>);
    static_assert(std::is_trivially_copyable_v<Packet>);
    static_assert(!std::is_trivially_copyable_v<NonTrivial>);

    std::cout << "sizeof(Packet)=" << sizeof(Packet) << '\n';
    std::cout << "alignof(Packet)=" << alignof(Packet) << '\n';
    std::cout << "offsetof(Packet.value)=" << offsetof(Packet, value) << '\n';
    std::cout << "sizeof(Empty)=" << sizeof(Empty) << '\n';
    std::cout << "sizeof(UsesEmptyBase)=" << sizeof(UsesEmptyBase) << '\n';
    std::cout << "sizeof(int)=" << sizeof(int) << '\n';
    std::cout << "L11_layout_observation OK\n";
}
