#pragma once
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace c11::dns {
enum class error { truncated, bad_label, pointer_loop, invalid_header, mismatch, no_answer };
template<class T> using result = std::expected<T, error>;
// This A-record teaching stub accepts ASCII hostname labels, not arbitrary DNS
// octet labels (SRV labels, binary labels and IDNA conversion are outside its API).
inline bool valid_label(std::string_view s) {
    if (s.empty() || s.size()>63 || s.front()=='-' || s.back()=='-') return false;
    for (unsigned char c:s) if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-')) return false;
    return true;
}
inline bool same_name(std::string_view a,std::string_view b) {
    if(a.size()!=b.size()) return false;
    const auto fold=[](char c){return c>='A'&&c<='Z'?static_cast<char>(c-'A'+'a'):c;};
    for(std::size_t i=0;i<a.size();++i) if(fold(a[i])!=fold(b[i])) return false;
    return true;
}
inline result<std::uint16_t> u16(std::span<const std::uint8_t> bytes, std::size_t at) {
    if (at > bytes.size() || bytes.size()-at < 2) return std::unexpected(error::truncated);
    return static_cast<std::uint16_t>((bytes[at] << 8) | bytes[at+1]);
}
struct name { std::string text; std::size_t next; };
// next refers to the original wire location after a compression pointer.
inline result<name> read_name(std::span<const std::uint8_t> bytes, std::size_t at) {
    if (bytes.size() > 4096) return std::unexpected(error::invalid_header);
    std::vector<bool> visited(bytes.size(), false);
    std::string text;
    std::size_t next = at;
    bool jumped = false;
    for (;;) {
        if (at >= bytes.size()) return std::unexpected(error::truncated);
        if (visited[at]) return std::unexpected(error::pointer_loop);
        visited[at] = true;
        const auto size = bytes[at++];
        if ((size & 0xc0) == 0xc0) {
            if (at >= bytes.size()) return std::unexpected(error::truncated);
            const auto target = static_cast<std::size_t>(((size & 0x3f) << 8) | bytes[at++]);
            const auto pointer_at=at-2;
            if(target>=pointer_at) return std::unexpected(target==pointer_at?error::pointer_loop:error::bad_label);
            if (!jumped) next = at;
            jumped = true; at = target; continue;
        }
        if (size & 0xc0) return std::unexpected(error::bad_label);
        if (!size) return name{std::move(text), jumped ? next : at};
        if (size > bytes.size()-at) return std::unexpected(error::truncated);
        if (!valid_label(std::string_view(reinterpret_cast<const char*>(bytes.data()+at),size))) return std::unexpected(error::bad_label);
        if (!text.empty()) text += '.';
        if (text.size()+size > 253) return std::unexpected(error::bad_label);
        text.append(reinterpret_cast<const char*>(bytes.data()+at), size);
        at += size;
    }
}
inline result<std::vector<std::uint8_t>> query(std::uint16_t id, std::string_view hostname) {
    if (hostname.empty() || hostname.size() > 253) return std::unexpected(error::bad_label);
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(id >> 8), static_cast<std::uint8_t>(id), 1,0,0,1,0,0,0,0,0,0};
    while (!hostname.empty()) {
        const auto dot = hostname.find('.');
        const auto label = hostname.substr(0, dot);
        if (!valid_label(label)) return std::unexpected(error::bad_label);
        out.push_back(static_cast<std::uint8_t>(label.size()));
        out.insert(out.end(), label.begin(), label.end());
        if (dot == std::string_view::npos) break;
        hostname.remove_prefix(dot+1);
        if (hostname.empty()) return std::unexpected(error::bad_label);
    }
    out.insert(out.end(), {0,0,1,0,1});
    return out;
}
inline result<std::string> parse_a(std::span<const std::uint8_t> bytes, std::uint16_t id, std::string_view expected_name) {
    if (bytes.size()<12 || bytes.size()>4096) return std::unexpected(error::invalid_header);
    if (*u16(bytes,0) != id) return std::unexpected(error::mismatch);
    const auto flags = *u16(bytes,2);
    if (!(flags & 0x8000) || (flags & 0x7800) || (flags & 0x0200) || (flags & 15) || *u16(bytes,4)!=1)
        return std::unexpected(error::invalid_header);
    auto question = read_name(bytes,12);
    if (!question) return std::unexpected(question.error());
    if (!same_name(question->text,expected_name)) return std::unexpected(error::mismatch);
    auto type = u16(bytes,question->next), cls = u16(bytes,question->next+2);
    if (!type || !cls) return std::unexpected(error::truncated);
    if (*type!=1 || *cls!=1) return std::unexpected(error::mismatch);
    std::size_t at = question->next+4;
    std::optional<std::string> candidate;
    const std::size_t answers=*u16(bytes,6);
    const std::size_t count=answers+*u16(bytes,8)+*u16(bytes,10);
    for (std::size_t i=0; i<count; ++i) {
        auto owner = read_name(bytes,at);
        if (!owner) return std::unexpected(owner.error());
        at = owner->next;
        if (at > bytes.size() || bytes.size()-at<10) return std::unexpected(error::truncated);
        const auto kind=*u16(bytes,at), group=*u16(bytes,at+2), length=*u16(bytes,at+8);
        at += 10;
        if (length>bytes.size()-at) return std::unexpected(error::truncated);
        if (kind==1 && group==1) {
            if (length!=4) return std::unexpected(error::invalid_header);
            if(i<answers && !candidate && same_name(owner->text,expected_name))
                candidate=std::to_string(bytes[at])+'.'+std::to_string(bytes[at+1])+'.'+std::to_string(bytes[at+2])+'.'+std::to_string(bytes[at+3]);
        }
        at += length;
    }
    if(at!=bytes.size()) return std::unexpected(error::invalid_header);
    if(candidate) return *candidate;
    return std::unexpected(error::no_answer);
}
}
