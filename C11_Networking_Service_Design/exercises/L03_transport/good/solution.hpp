#pragma once
#include <c11/dns.hpp>
#include <set>
#include <optional>
namespace exercise {
inline c11::dns::result<c11::dns::name> decode_name(std::span<const std::uint8_t> p, std::size_t at) {
    std::set<std::size_t> seen;
    std::optional<std::size_t> end;
    std::string output;
    if (p.size()>4096) return std::unexpected(c11::dns::error::invalid_header);
    for (;;) {
        if (at>=p.size()) return std::unexpected(c11::dns::error::truncated);
        if (!seen.insert(at).second) return std::unexpected(c11::dns::error::pointer_loop);
        auto n=p[at++];
        if ((n&0xc0)==0xc0) {
            if (at>=p.size()) return std::unexpected(c11::dns::error::truncated);
            const auto target=static_cast<std::size_t>(((n&63)<<8)|p[at]);
            if(target>=at-1) return std::unexpected(target==at-1?c11::dns::error::pointer_loop:c11::dns::error::bad_label);
            if (!end) end=at+1;
            at=target;
        } else {
            if (n&0xc0) return std::unexpected(c11::dns::error::bad_label);
            if (!n) return c11::dns::name{output,end.value_or(at)};
            if (n>p.size()-at) return std::unexpected(c11::dns::error::truncated);
            if (!c11::dns::valid_label(std::string_view(reinterpret_cast<const char*>(p.data()+at),n))) return std::unexpected(c11::dns::error::bad_label);
            if (!output.empty()) output+='.';
            if (output.size()+n>253) return std::unexpected(c11::dns::error::bad_label);
            for (unsigned i=0;i<n;++i) output+=static_cast<char>(p[at++]);
        }
    }
}
}
