#include <check.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>

struct Item {
    int value;
};

int main()
{
    float one = 1.0f;
    auto bits = std::bit_cast<std::uint32_t>(one);
    float round_trip = std::bit_cast<float>(bits);
    check(round_trip == one, "bit_cast round trip keeps the float value");

    Item source{42};
    alignas(Item) std::byte storage[sizeof(Item)]{};
    auto* target = std::construct_at(reinterpret_cast<Item*>(storage), Item{0});
    std::memcpy(target, &source, sizeof(Item));
    check(target->value == 42, "memcpy restores trivially copyable value into live target");
    std::destroy_at(target);

    auto* first = std::construct_at(reinterpret_cast<Item*>(storage), Item{7});
    check(first->value == 7, "first object is live");
    std::destroy_at(first);
    auto* second_raw = std::construct_at(reinterpret_cast<Item*>(storage), Item{9});
    auto* second = std::launder(second_raw);
    check(second->value == 9, "laundered pointer observes the replacement object");
    std::destroy_at(second);

    std::cout << "bit_pattern_1_0f=0x" << std::hex << bits << '\n';
    std::cout << "L13_aliasing_observation OK\n";
}
