#pragma once
#include <c11/dns.hpp>
namespace exercise {
inline c11::dns::result<c11::dns::name> decode_name(std::span<const std::uint8_t> p,std::size_t at) {
    auto value=c11::dns::read_name(p,at);
    // Deliberate parser defect: accepts a cyclic name as an empty name.
    if (!value && value.error()==c11::dns::error::pointer_loop) return c11::dns::name{"",at+2};
    return value;
}
}
