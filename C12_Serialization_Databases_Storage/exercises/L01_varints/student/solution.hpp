#pragma once
#include <c12/wire.hpp>
namespace solution {
inline c12::result<std::uint64_t> read(std::span<const std::byte>,std::size_t&){
    return std::unexpected(c12::error{c12::errc::unfinished});
}
}
