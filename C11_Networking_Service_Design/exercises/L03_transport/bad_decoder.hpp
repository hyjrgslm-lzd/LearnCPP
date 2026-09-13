#pragma once
#include <c11/dns.hpp>
#include <set>
#include <optional>
namespace exercise {
inline c11::dns::result<c11::dns::name> decode_name(std::span<const std::uint8_t> p,std::size_t at) {
    std::set<std::size_t> seen;std::optional<std::size_t> end;std::string text;
    if(p.size()>4096)return std::unexpected(c11::dns::error::invalid_header);
    for(;;) {
        if(at>=p.size())return std::unexpected(c11::dns::error::truncated);
        if(!seen.insert(at).second)return std::unexpected(c11::dns::error::pointer_loop);
        const auto size=p[at++];
        if((size&0xc0)==0xc0) {
            if(at>=p.size())return std::unexpected(c11::dns::error::truncated);
            const auto target=static_cast<std::size_t>(((size&63)<<8)|p[at]);
            if(target>=at-1)return std::unexpected(target==at-1?c11::dns::error::pointer_loop:c11::dns::error::bad_label);
#ifdef C11_BAD_NEXT
            end=at+1; // Bug: overwrites original wire consumption on a nested pointer.
#else
            if(!end)end=at+1;
#endif
            at=target;
        }else{
            if(size&0xc0)return std::unexpected(c11::dns::error::bad_label);
            if(!size)return c11::dns::name{text,end.value_or(at)};
            if(size>p.size()-at)return std::unexpected(c11::dns::error::truncated);
            if(!c11::dns::valid_label(std::string_view(reinterpret_cast<const char*>(p.data()+at),size)))return std::unexpected(c11::dns::error::bad_label);
            if(!text.empty())text+='.';
#ifndef C11_BAD_LENGTH
            if(text.size()+size>253)return std::unexpected(c11::dns::error::bad_label);
#endif
            text.append(reinterpret_cast<const char*>(p.data()+at),size);at+=size;
        }
    }
}
}
