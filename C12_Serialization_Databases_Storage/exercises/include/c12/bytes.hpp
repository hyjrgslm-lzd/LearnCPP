#pragma once
#include <c05/bytes.hpp>
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
namespace c12 {
using bytes = std::vector<std::byte>;
enum class errc { invalid, capacity, io, corrupt, outcome_unknown, unfinished, busy, allocation };
struct error { errc code; int native = 0; std::size_t offset = 0; };
template<class T> using result = std::expected<T, error>;
struct failure : std::runtime_error {
    error detail;
    failure(errc code, const char* message, int native = 0, std::size_t offset = 0)
        : std::runtime_error(message), detail{code,native,offset} {}
};
inline std::span<const std::byte> as_bytes(std::string_view s) { return std::as_bytes(std::span(s.data(),s.size())); }
template<std::unsigned_integral T> T number(std::span<const std::byte> in, std::size_t& at) {
    auto v=c05::read_be<T>(in,at);
    if(!v) throw failure(errc::corrupt,"incomplete integer",0,at);
    return *v;
}
inline void append(bytes& to,std::span<const std::byte> from) { to.insert(to.end(),from.begin(),from.end()); }
inline void text(bytes& to,std::string_view s) {
    if(s.size()>UINT32_MAX) throw failure(errc::capacity,"text width");
    c05::append_be(to,static_cast<std::uint32_t>(s.size())); append(to,as_bytes(s));
}
inline std::string text(std::span<const std::byte> in,std::size_t& at,std::size_t maximum) {
    auto count=number<std::uint32_t>(in,at);
    if(count>maximum) throw failure(errc::corrupt,"text limit",0,at);
    auto part=c05::take_bytes(in,at,count);
    if(!part) throw failure(errc::corrupt,"incomplete text",0,at);
    if(part->empty()) return {};
    return {reinterpret_cast<const char*>(part->data()),part->size()};
}
// CRC-32/ISO-HDLC detects accidental damage; it is not authentication.
inline std::uint32_t crc32(std::span<const std::byte> in) {
    std::uint32_t crc=UINT32_MAX;
    for(auto b:in) {
        crc^=std::to_integer<std::uint8_t>(b);
        for(int bit=0;bit!=8;++bit) crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return ~crc;
}
inline constexpr std::size_t max_keys=1024,max_key=256,max_value=4096,max_batch=256;
inline constexpr std::size_t max_log=64*1024*1024,max_payload=2*1024*1024;
using dictionary=std::map<std::string,std::string>;
struct mutation { std::string key; std::optional<std::string> value; };
inline result<dictionary> prepare_batch(const dictionary& current,std::span<const mutation> ops) {
    if(ops.empty()||ops.size()>max_batch) return std::unexpected(error{errc::invalid});
    try {
        auto next=current;
        for(const auto& op:ops) {
            if(op.key.empty()||op.key.size()>max_key||(op.value&&op.value->size()>max_value))
                return std::unexpected(error{errc::invalid});
            if(op.value) next[op.key]=*op.value; else next.erase(op.key);
            if(next.size()>max_keys) return std::unexpected(error{errc::capacity});
        }
        return next;
    } catch(const std::bad_alloc&) { return std::unexpected(error{errc::allocation}); }
}
inline bytes encode_operations(std::span<const mutation> ops) {
    bytes out; c05::append_be(out,static_cast<std::uint32_t>(ops.size()));
    for(const auto& op:ops) {
        out.push_back(op.value?std::byte{1}:std::byte{0}); text(out,op.key);
        if(op.value) text(out,*op.value);
    }
    return out;
}
inline std::vector<mutation> decode_operations(std::span<const std::byte> in,std::size_t limit=max_batch) {
    std::size_t at=0; auto count=number<std::uint32_t>(in,at);
    if(count>limit) throw failure(errc::corrupt,"operation count");
    std::vector<mutation> out; out.reserve(count);
    for(std::uint32_t n=0;n!=count;++n) {
        auto kind=number<std::uint8_t>(in,at);
        if(kind>1) throw failure(errc::corrupt,"operation kind");
        mutation op{text(in,at,max_key),{}};
        if(op.key.empty()) throw failure(errc::corrupt,"empty key");
        if(kind) op.value=text(in,at,max_value);
        out.push_back(std::move(op));
    }
    if(at!=in.size()) throw failure(errc::corrupt,"trailing operations",0,at);
    return out;
}
}
