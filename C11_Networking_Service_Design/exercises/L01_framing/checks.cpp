#include <solution.hpp>
#include <check.hpp>
#include <vector>
#include <iostream>

int main() {
    const std::vector<std::string> bodies{"", "hello", std::string("a\0b", 3), std::string(c11::max_frame, 'x')};
    std::string wire;
    for (const auto& b : bodies) wire += *c11::encode_frame(b);
    // Every split has the same interpretation, including empty calls/chunks.
    for (const auto chunk : {1u, 2u, 7u, 8u, 13u, 4096u}) {
        exercise::decoder decoder;
        std::vector<std::string> output;
        for (std::size_t offset = 0; offset < wire.size(); offset += chunk) {
            for (char c : std::string_view(wire).substr(offset, chunk)) {
                auto step = decoder.push(c);
                if (!step && step.error() == c11::frame_error::unfinished) {
                    std::cerr << "UNFINISHED: implement incremental frame decoder\n";
                    return 2;
                }
                check(step.has_value(), "valid byte accepted by student decoder");
                if (*step) output.push_back(std::move(**step));
                check(decoder.buffered() <= c11::max_frame + 8, "decoder retained bytes bounded");
            }
        }
        check(output == bodies, "fragmented and coalesced frames preserved");
        check(decoder.finish().has_value(), "EOF on boundary succeeds");
    }
    exercise::decoder oversized;
    bool refused = false;
    for (char c : std::string_view("00004097")) {
        const auto r = oversized.push(c);
        if (!r) { check(r.error() == c11::frame_error::too_large, "oversize error classified"); refused = true; break; }
    }
    check(refused, "oversized length rejected");
    check(!oversized.push('0'), "fatal parser error is sticky");
    for (const auto header : {"10000000", "00010000", "99999999"}) {
        exercise::decoder high_bits;
        bool rejected = false;
        for (char c : std::string_view(header)) {
            const auto r = high_bits.push(c);
            if (!r) { check(r.error() == c11::frame_error::too_large, "high digits error classified"); rejected = true; break; }
        }
        check(rejected, "high-order length digits cannot be discarded");
    }
    check(!c11::encode_frame(std::string(c11::max_frame + 1, 'x')), "encoder enforces same limit");
    for (const auto input : {"-0000001", "0000000x", " 0000001"}) {
        exercise::decoder bad;
        bool rejected = false;
        for (char c : std::string_view(input)) if (!bad.push(c)) { rejected = true; break; }
        check(rejected, "non-decimal header rejected");
    }
    for (const auto input : {"0", "00000003ab", "000000"}) {
        exercise::decoder short_frame;
        for (char c : std::string_view(input)) check(short_frame.push(c).has_value(), "partial frame accepted");
        auto eof = short_frame.finish();
        check(!eof && eof.error() == c11::frame_error::truncated, "EOF within frame rejected");
    }
    std::cout << "framing checks passed\n";
}
