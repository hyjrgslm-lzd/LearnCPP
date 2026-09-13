#pragma once
#include <c11/frame.hpp>
#include <c11/write_queue.hpp>

namespace c11 {
enum class connection_state { open, peer_eof, draining, closed };
class framed_connection {
    frame_decoder decoder_;
    write_queue output_;
    connection_state state_ = connection_state::open;
public:
    // Input chunks must be <=4096, admitted only while can_read(); the watermarks
    // reserve space for the incomplete frame plus this chunk's echo expansion.
    bool receive(std::string_view bytes) {
        if (!can_read() || bytes.size() > 4096) return false;
        for (char c : bytes) {
            auto step = decoder_.push(c);
            if (!step) { state_ = connection_state::closed; return false; }
            if (*step && !output_.enqueue(*encode_frame(**step))) {
                state_ = connection_state::closed; return false;
            }
        }
        return true;
    }
    bool eof() {
        if (state_ != connection_state::open) return false;
        const bool valid = decoder_.finish().has_value();
        state_ = valid && !output_.empty() ? connection_state::peer_eof : connection_state::closed;
        return valid;
    }
    void drain() noexcept {
        if (state_ == connection_state::closed) return;
        state_ = output_.empty() ? connection_state::closed : connection_state::draining;
    }
    bool can_read() const noexcept { return state_ == connection_state::open && !output_.paused(); }
    std::span<const char> output() const noexcept { return output_.front(); }
    void sent(std::size_t n) {
        output_.consume(n);
        if (state_ != connection_state::open && output_.empty()) state_ = connection_state::closed;
    }
    connection_state state() const noexcept { return state_; }
    std::size_t queued_bytes() const noexcept { return output_.bytes(); }
};
} // namespace c11
