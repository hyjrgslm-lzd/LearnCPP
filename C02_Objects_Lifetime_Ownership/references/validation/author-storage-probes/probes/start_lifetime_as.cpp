#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

int main()
{
#if defined(__cpp_lib_start_lifetime_as)
    alignas(std::uint32_t) std::array<std::byte, sizeof(std::uint32_t)> storage{
        std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}
    };
    auto* value = std::start_lifetime_as<std::uint32_t>(storage.data());
    auto* bytes = std::start_lifetime_as_array<std::byte>(value, sizeof(std::uint32_t));
    std::cout << "__cpp_lib_start_lifetime_as=" << __cpp_lib_start_lifetime_as << '\n';
    std::cout << "value=0x" << std::hex << *value << '\n';
    std::cout << "first_byte=0x" << std::hex << static_cast<unsigned>(std::to_integer<unsigned char>(bytes[0])) << '\n';
#else
#error "__cpp_lib_start_lifetime_as is not provided by this standard library"
#endif
}
