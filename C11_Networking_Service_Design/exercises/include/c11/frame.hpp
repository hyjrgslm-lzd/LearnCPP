#pragma once
#include <c11/frame_contract.hpp>
#include <utility>

namespace c11 {
// One complete frame is handed to the caller before another frame is accumulated.
class frame_decoder {
    std::size_t digits_ = 0;
    std::size_t length_ = 0;
    std::string body_;
    std::optional<frame_error> error_;
public:
    frame_step push(char byte) {
        if (error_) return std::unexpected(*error_);
        if (digits_ < 8) {
            if (byte < '0' || byte > '9') {
                error_ = frame_error::bad_length;
                return std::unexpected(*error_);
            }
            length_ = length_ * 10 + static_cast<unsigned>(byte - '0');
            ++digits_;
            if (length_ > max_frame) {
                error_ = frame_error::too_large;
                return std::unexpected(*error_);
            }
            if (digits_ != 8 || length_ != 0) return std::optional<std::string>{};
        } else {
            body_.push_back(byte);
            if (body_.size() != length_) return std::optional<std::string>{};
        }
        std::string complete = std::move(body_);
        body_.clear();
        digits_ = length_ = 0;
        return std::optional<std::string>{std::move(complete)};
    }
    std::expected<void, frame_error> finish() const {
        if (error_) return std::unexpected(*error_);
        if (digits_ != 0) return std::unexpected(frame_error::truncated);
        return {};
    }
    std::size_t buffered() const noexcept { return digits_ + body_.size(); }
};
} // namespace c11
