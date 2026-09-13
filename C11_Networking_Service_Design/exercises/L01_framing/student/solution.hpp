#pragma once
#include <c11/frame_contract.hpp>
namespace exercise {
class decoder {
public:
    c11::frame_step push(char) { return std::unexpected(c11::frame_error::unfinished); }
    std::expected<void, c11::frame_error> finish() const {
        return std::unexpected(c11::frame_error::unfinished);
    }
    std::size_t buffered() const noexcept { return 0; }
};
}
