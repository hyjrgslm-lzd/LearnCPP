#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size >= 4 && data[0] == 'C' && data[1] == '0' && data[2] == '1') {
        return data[3] == 0 ? 0 : 0;
    }
    return 0;
}
