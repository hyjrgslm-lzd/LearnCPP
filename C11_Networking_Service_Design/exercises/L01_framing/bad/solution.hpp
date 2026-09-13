#pragma once
#include <c11/frame_contract.hpp>
#include <charconv>
namespace exercise {
// Deliberate defect: trusts the peer's declared length without a resource limit.
class decoder {
    std::string bytes_;
    std::size_t length_ = 0;
public:
    c11::frame_step push(char c) {
        if (bytes_.size() < 8 && (c < '0' || c > '9'))
            return std::unexpected(c11::frame_error::bad_length);
        bytes_ += c;
        if (bytes_.size() == 8) std::from_chars(bytes_.data(), bytes_.data()+8, length_);
        if (bytes_.size() >= 8 && bytes_.size() == length_+8) {
            auto body = bytes_.substr(8);
            bytes_.clear();
            return std::optional<std::string>{std::move(body)};
        }
        return std::optional<std::string>{};
    }
    std::expected<void, c11::frame_error> finish() const {
        if (!bytes_.empty()) return std::unexpected(c11::frame_error::truncated);
        return {};
    }
    std::size_t buffered() const noexcept { return bytes_.size(); }
};
}
