#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>

namespace c05 {
enum class Errc {
    invalid_encoding, incomplete_input, invalid_number, out_of_range,
    limit_exceeded, invalid_value, duplicate_field, missing_field,
    unsupported_version, trailing_data, invalid_config, io_error, not_implemented
};
enum class OffsetUnit { byte, utf16_code_unit };
struct DataError {
    Errc code;
    std::size_t offset = 0;
    OffsetUnit unit = OffsetUnit::byte;
    std::string field;
    std::size_t line = 0;
};
inline constexpr std::size_t max_text_bytes = 4096;
inline constexpr std::size_t max_package_bytes = 1024 * 1024;
inline constexpr std::size_t max_records = 1024;
inline constexpr std::int64_t min_timestamp_ms = -62135596800000LL;
inline constexpr std::int64_t max_timestamp_ms = 253402300799999LL;
}
