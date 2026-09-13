#pragma once
#include <c12/wire.hpp>
namespace solution {
inline c12::result<std::uint64_t> read(std::span<const std::byte> input,std::size_t& cursor){
    return c12::read_varuint(input,cursor);
}
}
