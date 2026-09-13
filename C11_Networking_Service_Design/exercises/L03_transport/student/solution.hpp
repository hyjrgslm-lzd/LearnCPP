#pragma once
#include <c11/dns.hpp>
#include <stdexcept>
namespace exercise {
inline c11::dns::result<c11::dns::name> decode_name(std::span<const std::uint8_t>, std::size_t) {
    throw std::logic_error("UNFINISHED: bounded DNS name decompression");
}
}
