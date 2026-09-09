#pragma once

#include <string>
#include <string_view>

namespace l15::v2 {

enum class Format { legacy, compact };

class Engine {
public:
    std::string fetch(std::string_view name, int timeout_ms = 250,
        Format format = Format::compact) const
    {
        std::string mode = format == Format::legacy ? "legacy" : "compact";
        return std::string(name) + "|timeout=" + std::to_string(timeout_ms) + "|" + mode;
    }
};

} // namespace l15::v2

namespace l15::abi_model {

struct PublicOptionsV1 {
    int timeout_ms;
};

struct PublicOptionsV2 {
    int timeout_ms;
    bool compression_enabled;
};

inline constexpr bool public_layout_changed = sizeof(PublicOptionsV1) != sizeof(PublicOptionsV2);

} // namespace l15::abi_model
