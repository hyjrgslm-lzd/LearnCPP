#pragma once
#include "types.hpp"
#include <optional>
#include <vector>

namespace c05 {
struct ResourceRecord {
    std::uint32_t id{};
    std::string name;
    std::string path;
    std::uint64_t byte_size{};
    std::int64_t modified_at_ms{};
    std::optional<std::string> note;
    bool operator==(const ResourceRecord&) const = default;
};
struct Manifest {
    std::vector<ResourceRecord> records;
    bool operator==(const Manifest&) const = default;
};
enum class WireVersion : std::uint16_t { v1 = 1, v2 = 2 };
struct Config {
    std::string package_file = "manifest.c05m";
    std::string display_zone = "UTC";
    bool operator==(const Config&) const = default;
};
}
