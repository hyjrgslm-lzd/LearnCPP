#pragma once
#include <c12/wire.hpp>
namespace solution {
inline c12::result<std::uint64_t> read(std::span<const std::byte> input,std::size_t& cursor){
    auto probe=cursor;std::uint64_t out=0;
    for(unsigned n=0;n<10&&probe<input.size();++n){
        auto b=std::to_integer<unsigned>(input[probe++]);
        if(n==9&&(b&0xfe))return std::unexpected(c12::error{c12::errc::invalid});
        out|=static_cast<std::uint64_t>(b&127)<<(7*n);
        if(!(b&128)){cursor=probe;return out;} // Actual defect: accepts non-shortest representation.
    }return std::unexpected(c12::error{c12::errc::corrupt});
}
}
