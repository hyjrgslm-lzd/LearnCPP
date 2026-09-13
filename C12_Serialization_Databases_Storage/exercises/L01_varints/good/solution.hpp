#pragma once
#include <c12/wire.hpp>
namespace solution {
inline c12::result<std::uint64_t> read(std::span<const std::byte> input,std::size_t& cursor){
    auto position=cursor;std::uint64_t out=0;unsigned shift=0;
    while(position<input.size()&&shift<=63){
        const auto raw=std::to_integer<unsigned>(input[position++]);
        const auto payload=raw%128;
        if(shift==63&&payload>1)return std::unexpected(c12::error{c12::errc::invalid});
        out+=static_cast<std::uint64_t>(payload)<<shift;
        if(raw<128){
            if(shift&&payload==0)return std::unexpected(c12::error{c12::errc::invalid});
            cursor=position;return out;
        }
        shift+=7;
    }
    return std::unexpected(c12::error{c12::errc::corrupt});
}
}
