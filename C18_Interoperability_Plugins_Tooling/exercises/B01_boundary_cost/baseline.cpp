#include "c18/bytes.hpp"
#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

static bool parse(const char* text, size_t& value) {
    const std::string_view input(text);
    const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    return result.ec == std::errc{} && result.ptr == input.data() + input.size() && value > 0;
}

int main(int argc, char** argv) {
    size_t record_size = 16, records = 4096, iterations = 100;
    if ((argc != 4 && argc != 5) || !parse(argv[1], record_size) || !parse(argv[2], records) || !parse(argv[3], iterations) ||
        records > 65536 || record_size > 65536 || iterations > 10000 || record_size > (64 * 1024 * 1024) / records) {
        std::cerr << "usage: boundary_cost record_size records iterations [per_record|batch|copy_batch]\n";
        return 2;
    }
    const size_t count = record_size * records;
    if (iterations > (512u * 1024u * 1024u) / count) {
        std::cerr << "processed byte budget is 512 MiB per process\n"; return 2;
    }
    const std::string_view variant = argc == 5 ? argv[4] : "per_record";
    if (variant != "per_record" && variant != "batch" && variant != "copy_batch") return 2;
    std::vector<uint8_t> input(count), output(count), expected(count);
    std::vector<uint8_t> copied(variant == "copy_batch" ? count : 0);
    for (size_t i = 0; i < count; ++i) {
        input[i] = static_cast<uint8_t>((i * 73 + 17) % 256);
        expected[i] = input[i];
        if (expected[i] >= 97 && expected[i] <= 122) expected[i] = static_cast<uint8_t>(expected[i] - 32);
    }
    // Indirect-call cost is part of this experiment; volatile prevents a fixed inline-call-only driver.
    using Transform = c18_status (*)(const uint8_t*, size_t, uint8_t*, size_t, size_t*) noexcept;
    Transform volatile function = &c18::transform_bytes;
    size_t calls = 0;
    const auto started = std::chrono::steady_clock::now();
    for (size_t round = 0; round < iterations; ++round) {
        if (variant != "per_record") {
            const uint8_t* source = input.data();
            if (variant == "copy_batch") { std::memcpy(copied.data(), input.data(), count); source = copied.data(); }
            size_t written = 0;
            if (function(source, count, output.data(), count, &written) != C18_STATUS_OK || written != count) return 1;
            ++calls;
            continue;
        }
        for (size_t record = 0; record < records; ++record) {
            size_t written = 0;
            const auto offset = record * record_size;
            if (function(input.data() + offset, record_size, output.data() + offset, record_size, &written) != C18_STATUS_OK || written != record_size)
                return 1;
            ++calls;
        }
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    if (output != expected) { std::cerr << "check failed: benchmark byte oracle\n"; return 1; }
    std::cout << "{\"variant\":\"" << variant << "\",\"record_size\":" << record_size
              << ",\"records\":" << records << ",\"iterations\":" << iterations
              << ",\"calls\":" << calls << ",\"completed_bytes\":" << count * iterations
              << ",\"extra_copy_bytes\":" << (variant == "copy_batch" ? count * iterations : 0)
              << ",\"seconds\":" << elapsed << "}\n";
}
