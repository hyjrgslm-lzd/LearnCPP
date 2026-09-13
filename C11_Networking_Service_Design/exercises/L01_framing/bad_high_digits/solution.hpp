#pragma once
#include <c11/frame_contract.hpp>
#include <charconv>
namespace exercise {
class decoder {
    std::string bytes_;
    std::size_t length_ = 0;
    std::optional<c11::frame_error> error_;
public:
    c11::frame_step push(char c) {
        if (error_) return std::unexpected(*error_);
        if (bytes_.size() < 8 && (c < '0' || c > '9')) {
            error_ = c11::frame_error::bad_length;
            return std::unexpected(*error_);
        }
        bytes_ += c;
        if (bytes_.size() == 8) {
            // Deliberate bug: silently drops the high four decimal digits.
            std::from_chars(bytes_.data()+4, bytes_.data()+8, length_);
            if (length_ > c11::max_frame) { error_ = c11::frame_error::too_large; return std::unexpected(*error_); }
        }
        if (bytes_.size() >= 8 && bytes_.size() == length_+8) {
            auto result = bytes_.substr(8); bytes_.clear();
            return std::optional<std::string>{std::move(result)};
        }
        return std::optional<std::string>{};
    }
    std::expected<void, c11::frame_error> finish() const {
        if (error_) return std::unexpected(*error_);
        if (!bytes_.empty()) return std::unexpected(c11::frame_error::truncated);
        return {};
    }
    std::size_t buffered() const { return bytes_.size(); }
};
}
