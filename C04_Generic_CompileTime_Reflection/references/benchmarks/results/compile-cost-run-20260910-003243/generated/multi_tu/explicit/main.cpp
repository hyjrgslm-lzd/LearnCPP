#include <cstdint>
#include <cstdio>
extern "C" std::uint32_t use_0(std::uint32_t);
extern "C" std::uint32_t use_1(std::uint32_t);
extern "C" std::uint32_t use_2(std::uint32_t);
extern "C" std::uint32_t use_3(std::uint32_t);

std::uint32_t reference_transform(std::uint32_t value) {
    std::uint32_t result = value + 64u;
    for (int i = 0; i != 96; ++i) {
        result = (result * 31u) ^ (64u + static_cast<std::uint32_t>(i) * 17u);
    }
    return result;
}

int main() {
    const std::uint32_t inputs[4] = {1u, 2u, 3u, 4u};
    const std::uint32_t actual[4] = {use_0(inputs[0]), use_1(inputs[1]), use_2(inputs[2]), use_3(inputs[3])};
    const std::uint32_t expected[4] = {
        reference_transform(inputs[0] + 0u),
        reference_transform(inputs[1] + 1u),
        reference_transform(inputs[2] + 2u),
        reference_transform(inputs[3] + 3u)
    };
    std::printf("%u %u %u %u\n", actual[0], actual[1], actual[2], actual[3]);
    for (int i = 0; i != 4; ++i) {
        if (actual[i] != expected[i]) {
            return i + 1;
        }
    }
    return 0;
}
